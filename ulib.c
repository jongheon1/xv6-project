#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

char*
strcpy(char *s, const char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

int thread_create(void (*func)(void*), void* arg)
{
  printf(1, "thread start! func: %p \n", func);
    void *stack;
    int tid;

    // Allocate stack
    stack = malloc(4096);
    if (stack == 0) {
        return -1;
    }
    printf(1, "stack allocated! stack: %p \n", stack);
    // Align stack to page boundary
    // stack = (void *)PGROUNDUP((uint)stack);

    // Create new thread
    tid = clone(stack);
    printf(1, "cloned! tid: %d \n", tid);
    if (tid < 0) {
        free(stack);
        return -1;
    }

    // Start new thread
    if (tid == 0) {
        printf(1, "new thread! tid: %d \n", tid);
        func(arg);
        free(stack);
        exit();
    }

    return tid;


}

int thread_join(int tid)
{
    int join_tid;
    join_tid = join();
    
    if (join_tid == tid) {
        return 0;
    }
    
    return -1; 

}

