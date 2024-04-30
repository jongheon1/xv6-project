#include "types.h"
#include "stat.h"
#include "user.h"



void exit_test() {
  int pid1, pid2;

  pid1 = fork();
  if (pid1 == 0) {
    nice(2);
    printf(1, "Process 1 (pid: %d, priority: %d) exiting\n", getpid(), 2);
    exit();
  }

  pid2 = fork();
  if (pid2 == 0) {
    nice(1);
    printf(1, "Process 2 (pid: %d, priority: %d) exiting\n", getpid(), 1);
    exit();
  }

  wait();
  wait();
}

void sleep_test() {
  int pid1, pid2;

  pid1 = fork();
  if (pid1 == 0) {
    nice(1);
    printf(1, "Process 1 (pid: %d, priority: %d) sleeping\n", getpid(), 3);
    sleep(1);
    printf(1, "Process 1 (pid: %d, priority: %d) woke up\n", getpid(), 3);
    for (int i = 0; i < 5; i++) {
      printf(1, "Process 1 (pid: %d, priority: %d) running\n", getpid(), 3);
      yield();
    }
    exit();
  }

  pid2 = fork();
  if (pid2 == 0) {
    nice(2);
    printf(1, "Process 2 (pid: %d, priority: %d) running\n", getpid(), 4);
    for (int i = 0; i < 5; i++) {
      printf(1, "Process 2 (pid: %d, priority: %d) running\n", getpid(), 4);
      yield();
    }
    exit();
  }

  wait();
  wait();
}

void yield_test() {
  int pid1, pid2;

  pid1 = fork();
  if (pid1 == 0) {
    nice(2);
    for (int i = 0; i < 5; i++) {
      printf(1, "Process 1 (pid: %d, priority: %d) running\n", getpid(), 2);
      yield();
    }
    exit();
  }

  pid2 = fork();
  if (pid2 == 0) {
    nice(1);
    for (int i = 0; i < 5; i++) {
      printf(1, "Process 2 (pid: %d, priority: %d) running\n", getpid(), 1);
      yield();
    }
    exit();
  }

  wait();
  wait();
}

void nice_test() {
  int pid;

  pid = fork();
  if (pid == 0) {
    nice(-1);
    for (int i = 0; i < 5; i++) {
      printf(1, "Child process (pid: %d, priority: %d) running\n", getpid(), 1);
      yield();
    }
    nice(3);
    for (int i = 0; i < 5; i++) {
      printf(1, "Child process (pid: %d, priority: %d) running\n", getpid(), 4);
      yield();
    }

    exit();
  }
  nice(1);
  for (int i = 0; i < 5; i++) {
    printf(1, "Parent process (pid: %d, priority: %d) running\n", getpid(), 2);
    yield();
  }

  wait();
}

void wakeup_test() {
    int pid1, pid2;
    nice(-2);

    pid1 = fork();
    if (pid1 == 0) {
        nice(1);
        printf(1, "Process 1 (pid: %d, priority: %d) sleeping\n", getpid(), 2);
        sleep(10);
        printf(1, "Process 1 (pid: %d, priority: %d) woke up\n", getpid(), 2);
            for (int i = 0; i < 10; i++) {
      printf(1, "Process 1 (pid: %d, priority: %d) running\n", getpid(), 1);
      yield();
    }

        exit();
    }

    pid2 = fork();
    if (pid2 == 0) {
        printf(1, "Process 2 (pid: %d, priority: %d) sleeping\n", getpid(), 1);
        sleep(15);
        printf(1, "Process 2 (pid: %d, priority: %d) woke up\n", getpid(), 1);
            for (int i = 0; i < 5; i++) {
      printf(1, "Process 2 (pid: %d, priority: %d) running\n", getpid(), 1);
      yield();
    }

        exit();
    }

    sleep(15);
    wait();
    wait();
}

void blocking_test() {
    int pid1, pid2;

    pid1 = fork();
    if (pid1 == 0) {
        nice(1);
        printf(1, "Process 1 (pid: %d, priority: %d) reading\n", getpid(), 1);
        char buf[10];
        read(0, buf, sizeof(buf));
        printf(1, "Process 1 (pid: %d, priority: %d) finished reading\n", getpid(), 1);
        printf(1, "\n\n\n\n\n\n");
                for (int i = 0; i < 5; i++) {
            printf(1, "Process 1 (pid: %d, priority: %d) running\n", getpid(), 2);
        }

        exit();
    }

    pid2 = fork();
    if (pid2 == 0) {
        nice(2);
        for (int i = 0; i < 5; i++) {
            printf(1, "Process 2 (pid: %d, priority: %d) running\n", getpid(), 2);
            sleep(1);
        }
        exit();
    }

    wait();
    wait();
}


int main(int argc, char *argv[]) {
  printf(1, "==== Exit test ====\n");
  exit_test();

  printf(1, "\n==== Sleep test ====\n");
  sleep_test();

  printf(1, "\n==== Yield test ====\n");
  yield_test();

  printf(1, "\n==== Nice test ====\n");
  nice_test();

  printf(1, "\n==== Wakeup test ====\n");
  wakeup_test();

        printf(1, "\n==== Blocking Test ====\n");
    blocking_test();

  printf(1, "\nAll tests finished\n");
  exit();


}