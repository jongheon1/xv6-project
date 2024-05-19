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

int handle_mapped_file(struct proc *curproc, uint addr) {
    struct vma *vma = 0;
    for (int i = 0; i < 4; i++) {
        if (curproc->vmas[i].valid && curproc->vmas[i].start <= addr && addr < curproc->vmas[i].end) {
            vma = &curproc->vmas[i];
            break;
        }
    }
    if (vma == 0) {
        return 1;
    }

  // 물리 페이지 할당
  char *mem = kalloc();
  if (mem == 0) {
    return -1;  // 메모리 할당 실패
  }
  memset(mem, 0, PGSIZE);

  vma->file->off = vma->offset;
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

  // addr = PGROUNDDOWN(addr);
  if (mappages(curproc->pgdir, (void*)addr, PGSIZE, V2P(mem), perm) < 0) {
    kfree(mem);
    return -1;  // 페이지 매핑 실패
  }
  
  return 0;  // 매핑된 파일 처리 성공
}

int pagefault_handler(struct trapframe *tf) {
  uint addr = PGROUNDDOWN(rcr2());
  // addr = PGROUNDDOWN(addr);
  struct proc *curproc = myproc();

  // 페이지 폴트가 유효한 주소 범위 내에서 발생했는지 확인
  if (addr >= KERNBASE || addr < curproc->stack_guard) {
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
  int ret = handle_mapped_file(curproc, addr);
  if (ret <= 0) {
    return ret;
  }

  // 스택 영역에서 페이지 폴트가 발생한 경우
  if (addr < curproc->stack_bottom) {
    cprintf("Page fault: handled stack\n"); 
    if ((addr = allocuvm(curproc->pgdir, addr, addr + PGSIZE)) == 0) {
      return -1;
    }
    curproc->stack_bottom = addr - PGSIZE;
    return 0;
  }

  //나머지는 모두 힙 영역
  cprintf("Page fault: handled heap\n");
  if ((addr = allocuvm(curproc->pgdir, addr, addr + PGSIZE)) == 0) {
    return -1;
  }
  curproc->sz = addr;
  
  return 0;
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
    cprintf("page_fault pid %d %s: trap %d err %d on cpu %d "
        "eip 0x%x addr 0x%x\n",
        myproc()->pid, myproc()->name, tf->trapno, tf->err, cpuid(), tf->eip, rcr2());

    int res = pagefault_handler(tf);
    if (res == -1) {
      // 잘못된 메모리 접근으로 인한 페이지 폴트인 경우 프로세스 종료
      cprintf("pid %d %s: trap %d err %d on cpu %d "
              "eip 0x%x addr 0x%x--kill proc\n",
              myproc()->pid, myproc()->name, tf->trapno, tf->err, cpuid(), tf->eip, rcr2());
      myproc()->killed = 1;
      exit();
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
