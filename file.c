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

int find_free_vma_slot(struct proc *p) {
  for (int i = 0; i < 4; i++) {
    if (!p->vmas[i].valid) {
      return i;
    }
  }
  return -1;
}

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
  return 0;
}

void remove_vma(struct proc* p, uint start, uint end) {
  for (int i = 0; i < 4; i++) {
    if (p->vmas[i].valid && p->vmas[i].start == start && p->vmas[i].end == end) {
      p->vmas[i].valid = 0;
      
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

  if (f == 0 || !f->readable || off < 0 || len <= 0 || off % PGSIZE != 0) {
    return -1;
  }

  uint start = find_free_addr_range(p, len);
  if (start == 0) {
    return -1;
  }

  if (add_vma(p, start, start + len, flags, f, off) < 0) {
    return -1;
  }
  // for (uint a = start; a < start + len; a += PGSIZE) {
  //   char *mem = kalloc();
  //   if (mem == 0) {
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
  //   if (mappages(p->pgdir, (void*)a, PGSIZE, V2P(mem), pte_flags) < 0) {
  //     kfree(mem);
  //     unmap_pages(p->pgdir, start, a);
  //     remove_vma(p, start, start + len);
  //     unmap_file(f);
  //     global_nvmas--;
  //     return -1;
  //   }
  //   fileread(f, mem, PGSIZE);
  //   //offset reset
  //    f->off = off;
  // }

  return start;
}

int munmap(void* addr, int len) {
    struct proc *p = myproc();

    if (addr == 0 || (uint)addr % PGSIZE != 0) {
        return -1;
    }

    for (int i = 0; i < 4; i++) {
      if (p->vmas[i].valid && 
      ((p->vmas[i].start <= (uint)addr && (uint)addr < p->vmas[i].end) ||
      (p->vmas[i].start < (uint)addr + len && (uint)addr + len <= p->vmas[i].end)
      )) {
        if (p->vmas[i].start == (uint)addr && p->vmas[i].end == (uint)addr + len) {
            if (p->vmas[i].file != 0 && (p->vmas[i].prot & MAP_PROT_WRITE)) {              
              filewrite(p->vmas[i].file, (char*)p->vmas[i].start, p->vmas[i].end - p->vmas[i].start);
            }
            p->vmas[i].valid = 0;
            unmap_pages(p->pgdir, (uint)addr, (uint)addr + len);
            switchuvm(p);
            return 0;
        } else {
          return -1;
        }
      }
    }
    return 0;
}

