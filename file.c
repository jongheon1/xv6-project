//
// File descriptors
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"



struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

struct file_mapping {
    struct file* file;
    int mapped; // 0: not mapped, 1: mapped
};



#define	MAP_FAILED	((void	*)	-1)
#define	MAP_PROT_READ 0x00000001
#define	MAP_PROT_WRITE 0x00000002


void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{

  struct proc *curproc = myproc();

  for (int i = 0; i < 4; i++) {
    if (curproc->vmas[i].valid && curproc->vmas[i].file == f) {
      munmap((void *)curproc->vmas[i].start, curproc->vmas[i].end - curproc->vmas[i].start);
    }
  }

  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_PIPE)
    pipeclose(ff.pipe, ff.writable);
  else if(ff.type == FD_INODE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// Get metadata about file f.
int
filestat(struct file *f, struct stat *st)
{
  if(f->type == FD_INODE){
    ilock(f->ip);
    stati(f->ip, st);
    iunlock(f->ip);
    return 0;
  }
  return -1;
}

// Read from file f.
int
fileread(struct file *f, char *addr, int n)
{
  int r;

  if(f->readable == 0)
    return -1;
  if(f->type == FD_PIPE)
    return piperead(f->pipe, addr, n);
  if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
    return r;
  }
  panic("fileread");
}

//PAGEBREAK!
// Write to file f.
int
filewrite(struct file *f, char *addr, int n)
{
  int r;

  if(f->writable == 0)
    return -1;
  if(f->type == FD_PIPE) 
    return pipewrite(f->pipe, addr, n);
  if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * 512;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();

      if(r < 0)
        break;
      if(r != n1)
        panic("short filewrite");
      i += r;
    }
    return i == n ? n : -1;
  }
  panic("filewrite");
}



uint find_free_addr_range(struct proc *p, int len) {
  uint addr = KERNBASE - PGSIZE;

  for (; addr >= PGSIZE; addr -= PGSIZE) {
    int found = 1;
    for (int i = 0; i < 4; i++) {
      if (p->vmas[i].valid) {
        uint start = addr;
        uint end = addr + len;
        if ((start >= p->vmas[i].start && start < p->vmas[i].end) ||
            (end > p->vmas[i].start && end <= p->vmas[i].end) ||
            (start < p->vmas[i].start && end > p->vmas[i].end)) {
          found = 0;
          addr = p->vmas[i].start - PGSIZE;
          break;
        }
      }
    }
    if (found) {
      return addr;
    }
  }

  return 0;
}

// 프로세스의 VMA 배열에서 빈 슬롯을 찾아 인덱스를 반환합니다.
int find_free_vma_slot(struct proc *p) {
  for (int i = 0; i < 4; i++) {
    if (!p->vmas[i].valid) {
      return i;
    }
  }
  return -1;
}

// 프로세스의 VMA 배열에 새로운 VMA를 추가합니다.
int add_vma(struct proc *p, uint start, uint end, int prot, struct file *file, uint offset) {
  int slot = find_free_vma_slot(p);
  if (slot < 0) {
    return -1;
  }
  p->vmas[slot].start = start;
  p->vmas[slot].end = end;
  p->vmas[slot].prot = prot;
  p->vmas[slot].file = file;
  p->vmas[slot].offset = offset;
  p->vmas[slot].valid = 1;
  p->nvmas++;
  return 0;
}

// 프로세스의 VMA 배열에서 주어진 주소에 해당하는 VMA를 찾아 인덱스를 반환합니다.
int find_vma(struct proc *p, uint addr) {
  for (int i = 0; i < 4; i++) {
    if (p->vmas[i].valid && p->vmas[i].start <= addr && addr < p->vmas[i].end) {
      return i;
    }
  }
  return -1;
}

void remove_vma(struct proc* p, uint start, uint end) {
  for (int i = 0; i < 4; i++) {
    if (p->vmas[i].valid && p->vmas[i].start == start && p->vmas[i].end == end) {
      // VMA를 무효화하고 프로세스의 VMA 배열에서 제거합니다.
      p->vmas[i].valid = 0;
      p->nvmas--;
      
      break;
    }
  }
}


void unmap_pages(pde_t *pgdir, uint start, uint end) {
  for (uint a = start; a < end; a += PGSIZE) {
    pte_t *pte = walkpgdir(pgdir, (void*)a, 0);
    if (pte && (*pte & PTE_P)) {
      kfree((char*)P2V(PTE_ADDR(*pte)));
      *pte = 0;
    }
  }
}

int mmap(struct file* f, int off, int len, int flags)
{
	  struct proc *p = myproc();

  // 파일과 오프셋, 길이 값을 검증합니다.
  if (f == 0 || !f->readable || off < 0 || len <= 0 || off % PGSIZE != 0) {
    return -1;
  }

  // VMA의 시작 주소를 찾습니다. (구현해야 함)
  uint start = find_free_addr_range(p, len);
  if (start == 0) {
    return -1;
  }

  // VMA를 생성하고 프로세스의 VMA 배열에 추가합니다.
  if (add_vma(p, start, start + len, flags, f, off) < 0) {
    return -1;
  }
  f->off = off;

  // for (uint a = start; a < start + len; a += PGSIZE) {
  //   //매핑된 파일을 올리기 위한 물리 메모리 할당
  //   char *mem = kalloc();
  //   if (mem == 0) {
  //     // 메모리 할당 실패 시 이전에 할당한 페이지 해제와 VMA 제거
  //     unmap_pages(p->pgdir, start, a);
  //     remove_vma(p, start, start + len);
  //     unmap_file(f);
  //     global_nvmas--;
  //     return -1;
  //   }
  //   memset(mem, 0, PGSIZE);

  //   int pte_flags = PTE_U;
  //   if (flags & MAP_PROT_WRITE) {
  //     pte_flags |= PTE_W;
  //   }
  //   //가상 주소가 할당 받은 물리 메모리를 참조할 수 있도록 페이지 테이블 생성
  //   if (mappages(p->pgdir, (void*)a, PGSIZE, V2P(mem), pte_flags) < 0) {
  //     // 페이지 매핑 실패 시 할당한 페이지와 VMA 제거
  //     kfree(mem);
  //     unmap_pages(p->pgdir, start, a);
  //     remove_vma(p, start, start + len);
  //     unmap_file(f);
  //     global_nvmas--;
  //     return -1;
  //   }
  //   //이제 물리 메모리(mem)에 파일 올려놓으면 가상 메모리에 접근 가능
  //   fileread(f, mem, PGSIZE);
  //   //offset reset
  //    f->off = off;
  // }

  return start;
}

int munmap(void* addr, int len) {
    struct proc *p = myproc();

    // 주소 값을 검증합니다.
    if (addr == 0 || (uint)addr % PGSIZE != 0) {
        return -1;
    }

    // 해당 주소 범위와 정확히 일치하는 VMA를 찾습니다.
    for (int i = 0; i < 4; i++) {
        if (p->vmas[i].valid && p->vmas[i].start == (uint)addr && p->vmas[i].end == (uint)addr + len) {
            // 파일 백킹 VMA인 경우 dirty page를 파일에 씁니다.
            if (p->vmas[i].file != 0 && (p->vmas[i].prot & MAP_PROT_WRITE)) {              
              filewrite(p->vmas[i].file, (char*)p->vmas[i].start, p->vmas[i].end - p->vmas[i].start);
            }
            // VMA를 무효화하고 프로세스의 VMA 배열에서 제거합니다.
            p->vmas[i].valid = 0;
            p->nvmas--;
            // 해당 주소 범위의 페이지를 해제합니다.
            unmap_pages(p->pgdir, (uint)addr, (uint)addr + len);
            return 0;
        }
    }
    return -1;
}

