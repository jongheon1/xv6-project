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
    int fd, i;
    char *addr;

    printf(1, "test start ! \n");
  
    // Open file
    if ((fd = open("README", O_RDWR | O_CREATE)) < 0) {
        printf(1, "open failed\n");
        exit();
    }

    printf(1, "open success ! \n");

    for (i = 0; i < 20; i++){
        write(fd, "a", 1);
    }
    printf(1, "write success ! \n");

    // Test 1: mmap with read/write flag
    addr = mmap(fd, 0, 20, PROT_READ | PROT_WRITE);
    if (addr == MAP_FAILED) {
        printf(1, "mmap failed\n");
        exit();
    }
    printf(1, "mmap addr: %p\n", addr);

    // 매핑된 메모리 영역의 내용 출력
    printf(1, "Read from mmap: ");
    for (i = 0; i < 10; i++) {
    printf(1, "%c", addr[5 + i]);
    }
    printf(1, "\n");

    // 매핑된 메모리 영역 수정
    addr[0] = 'b';

    // Test 2: munmap
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

    // // 파일 내용 확인
    // char buf[2];
    // if (read(fd, buf, 1) > 0) {
    // printf(1, "file content after munmap: %c\n", buf[0]);
    // } else {
    // printf(1, "read failed\n");
    // }

    close(fd);
  
    printf(1, "mmap and munmap test finished\n");
    exit();
}
