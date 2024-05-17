#include "types.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "file.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

int grow_stack(struct proc *curproc, uint addr) {
  // 스택 크기 제한 확인
  // if (addr < curproc->stack_base - PGSIZE) {
  //   return -1;  // 스택 크기 제한을 초과한 경우 확장 불가
  // }
  // // 새로운 물리 페이지 할당
  // char *mem = kalloc();
  // if (mem == 0) {
  //   return -1;  // 메모리 할당 실패
  // }
  // // 할당한 물리 페이지 초기화
  // memset(mem, 0, PGSIZE);
  // // 페이지 테이블에 매핑
  // if (mappages(curproc->pgdir, (char*)PGROUNDDOWN(addr), PGSIZE, V2P(mem), PTE_W|PTE_U) < 0) {
  //   kfree(mem);
  //   return -1;  // 페이지 매핑 실패
  // }
  // // 스택 포인터 업데이트
  // curproc->stack_base -= PGSIZE;
  return 0;  // 스택 확장 성공
}

int grow_heap(struct proc *curproc, uint addr) {
  char* mem;

  addr = PGROUNDDOWN(addr);

  mem = kalloc();
  if (mem == 0) {
    return -1;  // 메모리 할당 실패
  }
  memset(mem, 0, PGSIZE);
  if(mappages(curproc->pgdir, (void*)addr, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0) {
    kfree(mem);
    return -1;  // 페이지 매핑 실패
  }

  return 0;  // 힙 확장 성공
}

int handle_mapped_file(struct proc *curproc, uint addr) {
    struct vma *vma = 0;
    for (int i = 0; i < 4; i++) {
        if (curproc->vmas[i].valid && curproc->vmas[i].start <= addr && addr < curproc->vmas[i].end) {
            vma = &curproc->vmas[i];
            break;
        }
    }
    if (vma == 0) {
        return -1;
    }

  // 물리 페이지 할당
  char *mem = kalloc();
  if (mem == 0) {
    return -1;  // 메모리 할당 실패
  }
  memset(mem, 0, PGSIZE);

  int bytes = fileread(vma->file, mem, PGSIZE);
  if (bytes < 0) {
    kfree(mem);
    return -1;  // 파일 읽기 실패
  }
  vma->file->off = vma->offset;

  // 페이지 테이블에 매핑
  int perm = PTE_U;
  if (vma->prot & PTE_W) {
    perm |= PTE_W;
  }

  addr = PGROUNDDOWN(addr);
  if (mappages(curproc->pgdir, (void*)addr, PGSIZE, V2P(mem), perm) < 0) {
    kfree(mem);
    return -1;  // 페이지 매핑 실패
  }
  
  return 0;  // 매핑된 파일 처리 성공
}

int pagefault_handler(struct trapframe *tf) {
  uint addr = rcr2();
  struct proc *curproc = myproc();

  // 페이지 폴트가 유효한 주소 범위 내에서 발생했는지 확인
  if (addr >= KERNBASE) {
    return -1;  // 잘못된 메모리 접근
  }

  // 쓰기 작업으로 인한 폴트인 경우
  if (tf->err & 0x2) {
    pte_t *pte = walkpgdir(curproc->pgdir, (void *)addr, 0);
    if (pte && (*pte & PTE_P) && !(*pte & PTE_W)) {
      return -1;  // 읽기 전용 페이지에 쓰기 작업 시도
    }
  }
  // 매핑된 파일 영역에서 페이지 폴트가 발생한 경우 처리
  if (handle_mapped_file(curproc, addr) == 0) {
      return 0;  // 정상 처리
  }

  // 힙 영역에서 페이지 폴트가 발생한 경우
  
  if (addr < curproc->sz) {
    if (grow_heap(curproc, addr) == 0) {
      return 0;  // 힙 확장 성공
    }  
  }

  // // 스택 영역에서 페이지 폴트가 발생한 경우



  return -1;  // 처리되지 않은 페이지 폴트
}


//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }
  if (tf->trapno == T_PGFLT) {
    int res = pagefault_handler(tf);
    if (res == -1) {
      // 잘못된 메모리 접근으로 인한 페이지 폴트인 경우 프로세스 종료
      cprintf("pid %d %s: trap %d err %d on cpu %d "
              "eip 0x%x addr 0x%x--kill proc\n",
              myproc()->pid, myproc()->name, tf->trapno, tf->err, cpuid(), tf->eip, rcr2());
      myproc()->killed = 1;
      return;
    }
    // Demand paging 처리 후 원래 실행 흐름으로 돌아감
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock); 
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
//  case T_IRQ0 + IRQ_IDE2:
//	ide2intr();
//	lpaiceoi();
//	break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
