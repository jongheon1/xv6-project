#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

#define PGSIZE 4096
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_FAILED ((void*)-1)

int main(int argc, char *argv[]) {
//   int stack_var = 0;
//   char *heap_ptr = 0;

//   printf(1, "Lazy Allocation Test\n");

//   // 스택 확장 테스트
//   printf(1, "Stack Allocation Test\n");
//   printf(1, "Current stack variable address: %p\n", &stack_var);

// //   스택 포인터를 감소시켜 스택 확장 유도
//   int *stack_ptr = &stack_var;
//   for (int i = 0; i < 2; i++) {
//     stack_ptr -= PGSIZE / sizeof(int);
//     printf(1, "Accessing stack address: %p\n", stack_ptr);
//     *stack_ptr = i;
//   }

//   // 힙 확장 테스트
//   printf(1, "Heap Allocation Test\n");
//   printf(1, "Current heap pointer: %p\n", heap_ptr);

//   // sbrk를 사용하여 힙 확장 유도
//   for (int i = 0; i < 4; i++) {
//     heap_ptr = sbrk(PGSIZE);
//     printf(1, "Allocated heap memory: %p\n", heap_ptr);
//     *heap_ptr = i;
//   }

//   // 결과 출력
//   printf(1, "Stack Allocation Result\n");
//   stack_ptr = &stack_var;
//   for (int i = 0; i < 2; i++) {
//     stack_ptr -= PGSIZE / sizeof(int);
//     printf(1, "Stack address: %p, Value: %d\n", stack_ptr, *stack_ptr);
//   }

//   printf(1, "Heap Allocation Result\n");
//   heap_ptr = sbrk(0) - 4 * PGSIZE;
//   for (int i = 0; i < 4; i++) {
//     printf(1, "Heap address: %p, Value: %d\n", heap_ptr, *heap_ptr);
//     heap_ptr += PGSIZE;
//   }

//   printf(1, "Lazy Allocation Test Finished\n");
//   exit();

    int fd, i;
    char *addr;

    printf(1, "test start ! \n");
  
    // Open file
    if ((fd = open("TRICKS", O_RDWR | O_CREATE)) < 0) {
        printf(1, "open failed\n");
        exit();
    }

    printf(1, "open success ! \n");

    for (i = 0; i < 10; i++){
        write(fd, "a", 1);
    }
    printf(1, "write success ! \n");

    // Test 1: mmap with read/write flag
    addr = mmap(fd, 0, PGSIZE, PROT_READ | PROT_WRITE);
    if (addr == MAP_FAILED) {
        printf(1, "mmap failed\n");
        exit();
    }
    printf(1, "mmap addr: %p\n", addr);

    // 매핑된 메모리 영역의 내용 출력
    printf(1, "Read from mmap: ");
    for (i = 0; i < 10; i++) {
    printf(1, "%c", addr[i]);
    }
    printf(1, "\n");

    // 매핑된 메모리 영역 수정
    addr[0] = 'b';

    // Test 2: munmap
    if (munmap(addr + PGSIZE, PGSIZE) < 0) {
        printf(1, "munmap 1 failed\n");
        // exit();
    }
    printf(1, "munmap 1 success\n");
    if (munmap(addr + 20, PGSIZE) < 0) {
        printf(1, "munmap 2 failed\n");
        // exit();
    }
    printf(1, "munmap 2 success\n");

    if (munmap(addr, PGSIZE) < 0) {
      printf(1, "munmap failed\n");
      exit();
    }
    printf(1, "munmap success\n");


    addr = mmap(fd, 0, PGSIZE, PROT_READ | PROT_WRITE);
    if (addr == MAP_FAILED) {
        printf(1, "mmap failed\n");
        exit();
    }
    printf(1, "mmap addr: %p\n", addr);

    // 매핑된 메모리 영역의 내용 출력
    printf(1, "Read from mmap: ");
    for (i = 0; i < 10; i++) {
    printf(1, "%c", addr[i]);
    }
    printf(1, "\n");

    // Test 2: munmap
    if (munmap(addr, PGSIZE) < 0) {
        printf(1, "munmap failed\n");
        exit();
    }
    printf(1, "munmap success\n");

    close(fd);
  
    printf(1, "mmap and munmap test finished\n");
    exit();
}
