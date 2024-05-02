#include "types.h"
#include "stat.h"
#include "user.h"


void mlfq_test()
{
  volatile unsigned int sum = 0;
  unsigned int i;
  int pid1, pid2;
  int long_start, long_end, short_start, short_end, inter_start, inter_end;

  // Long-running process
  long_start = uptime();
  for (i=0; i<200000000; i++) {
    sum += i;
    if (i == 80000000) {
      short_start = uptime();
            printf(1, "Short process start 1\n");

      pid1 = fork();

      if (pid1 == 0) {
            printf(1, "Short process start 2\n");

        // Short-running process
        volatile unsigned int short_sum = 1;
        unsigned int j;
        for (j=0; j<20000000; j++) {
          short_sum *= j;
        }
        short_end = uptime();
        printf(1, "Short process turnaround time: %d\n",  short_end - short_start);
        exit();
      } 

    } else if (i == 160000000) {
      inter_start = uptime();
                    printf(1, "Interactive process start 1\n");

      pid2 = fork();
      if (pid2 == 0) {

              printf(1, "Interactive process start 2\n");

        // Interactive process
        for (int k=1; k<5; k++) {
	        sleep(1);
        }
        inter_end = uptime();
        printf(1, "Interactive process turnaround time: %d\n", inter_end - inter_start);
        exit();
      }
    }
    
  }

  long_end = uptime();
            //   ps();
            // printQueue();

  printf(1, "Long process turnaround time: %d\n", long_end - long_start);
  wait();
  wait();
}

void fork_test()
{
    int pid;

    pid = fork();
    if (pid == 0) {
        printf(1, "Child process (pid: %d) is created\n", getpid());
        exit();
    } else {
        printf(1, "Parent process (pid: %d) is running\n", getpid());
        wait();
    }
}


void yield_test()
{
    int pid;

    pid = fork();
    if (pid == 0) {
        printf(1, "Child process (pid: %d) is yielding\n", getpid());
        yield();
        printf(1, "Child process (pid: %d) resumed\n", getpid());
        exit();
    } else {
        printf(1, "Parent process (pid: %d) is running\n", getpid());
        wait();
    }
}

void wakeup_test()
{
    int pid;

    pid = fork();
    if (pid == 0) {
        printf(1, "Child process (pid: %d) is sleeping\n", getpid());
        sleep(5);
        printf(1, "Child process (pid: %d) woke up\n", getpid());
        exit();
    } else {
        sleep(2);
        printf(1, "Parent process (pid: %d) is waking up child\n", getpid());
        kill(pid);
        wait();
    }
}

void time_slice_test()
{
    int pid;

    pid = fork();
    if (pid == 0) {
        printf(1, "Child process (pid: %d) is running\n", getpid());
        printQueue();
        for (int i = 0; i < 20; i++) {
            printf(1, "Child process (pid: %d) is running (iteration: %d)\n", getpid(), i);
        }
        exit();
    } else {
        printf(1, "Parent process (pid: %d) is running\n", getpid());
        printQueue();
        for (int i = 0; i < 10; i++) {
            printf(1, "Parent process (pid: %d) is running (iteration: %d)\n", getpid(), i);
        }
        wait();
    }
}

void wakeup_priority_test() {
    int pid1, pid2;

    pid1 = fork();
    if (pid1 == 0) {
        nice(2);
        printf(1, "Process 1 (pid: %d, priority: %d) sleeping\n", getpid(), 2);
        sleep(10);
        printf(1, "Process 1 (pid: %d, priority: %d) woke up\n", getpid(), 2);
        exit();
    }

    pid2 = fork();
    if (pid2 == 0) {
        nice(1);
        printf(1, "Process 2 (pid: %d, priority: %d) sleeping\n", getpid(), 1);
        sleep(5);
        printf(1, "Process 2 (pid: %d, priority: %d) woke up\n", getpid(), 1);
        exit();
    }

    sleep(15);
    wait();
    wait();
}

void same_priority_test() {
    int pid1, pid2, pid3;

    pid1 = fork();
    if (pid1 == 0) {
        // ps();
        // printQueue();
        for (int i = 0; i < 10; i++) {
            volatile unsigned int k = 1;
            for (int j = 0; j < 100000000; j++) {
                k *= j *123213;
            }
            printf(1, "Process 1 (pid: %d, priority: %d) running\n", getpid(), 2);
            printQueue();
            // yield();
        }
        exit();
    }

    pid2 = fork();
    if (pid2 == 0) {
        // ps();
        // printQueue();
        for (int i = 0; i < 10; i++) {
            volatile unsigned int k = 1;
            for (int j = 0; j < 100000000; j++) {
                k *= j *123123;
            }
            printf(1, "Process 2 (pid: %d, priority: %d) running\n", getpid(), 2);
            printQueue();
            // yield();
        }
        exit();
    }

    pid3 = fork();
    if (pid3 == 0) {
        // ps();
        // printQueue();
        for (int i = 0; i < 10; i++) {
            volatile unsigned int k = 1;
            for (int j = 0; j < 100000000; j++) {
                k *= j *132123;
            }
            printf(1, "Process 3 (pid: %d, priority: %d) running\n", getpid(), 2);
            printQueue();
            // yield();
        }
        exit();
    }

    wait();
    wait();
    wait();
}

void dynamic_queue_test() {
    int pid1, pid2, pid3;

    printf(1, "==== Dynamic Queue Test ====\n");

    pid1 = fork();
    if (pid1 == 0) {
        printf(1, "Child process 1 (pid: %d) created\n", getpid());
        printQueue();
        for (int i = 0; i < 5; i++) {
            printf(1, "Child process 1 (pid: %d) running (iteration: %d)\n", getpid(), i);
            printQueue();
            sleep(1);
        }
        printf(1, "Child process 1 (pid: %d) exiting\n", getpid());
        printQueue();
        exit();
    }

    pid2 = fork();
    if (pid2 == 0) {
        printf(1, "Child process 2 (pid: %d) created\n", getpid());
        printQueue();
        for (int i = 0; i < 3; i++) {
            printf(1, "Child process 2 (pid: %d) running (iteration: %d)\n", getpid(), i);
            printQueue();
            sleep(2);
        }
        printf(1, "Child process 2 (pid: %d) exiting\n", getpid());
        printQueue();
        exit();
    }

    pid3 = fork();
    if (pid3 == 0) {
        printf(1, "Child process 3 (pid: %d) created\n", getpid());
        printQueue();
        for (int i = 0; i < 4; i++) {
            printf(1, "Child process 3 (pid: %d) running (iteration: %d)\n", getpid(), i);
            printQueue();
            if (i == 1) {
                printf(1, "Child process 3 (pid: %d) yielding\n", getpid());
                printQueue();
                yield();
            }
            sleep(1);
        }
        printf(1, "Child process 3 (pid: %d) exiting\n", getpid());
        printQueue();
        exit();
    }

    // Wait for child processes to finish
    for (int i = 0; i < 3; i++) {
        wait();
    }

    printf(1, "==== Dynamic Queue Test Finished ====\n");
}
void priority_selection_test() {
    int pid[10];
    int i;

    printf(1, "==== Priority Selection Test ====\n");

    // Create multiple processes
    for (i = 0; i < 8; i++) {
        pid[i] = fork();
        if (pid[i] == 0) {
            printf(1, "Process %d (pid: %d) created\n", i, getpid());
            // printQueue();

            // Perform some work to consume time slices
            for (int j = 0; j < 4; j++) {
                printf(1, "Process %d (pid: %d) running (iteration: %d)\n", i, getpid(), j);
                printQueue();
                volatile unsigned int k = 1;
                for (int j = 0; j < 1000000; j++) {
                    k *= j *13212;
                }

                sleep(1);

            }

            printf(1, "Process %d (pid: %d) exiting\n", i, getpid());
            printQueue();
            exit();
        }
    }

    // Wait for child processes to finish
    for (i = 0; i < 10; i++) {
        wait();
    }

    printf(1, "==== Priority Selection Test Finished ====\n");
}


int main(int argc, char **argv)
{
    // printf(1, "===Wakeup")
//   printf(1, "====same_priority_test====\n");
//   dynamic_queue_test();

      printf(1, "--- mlfq Test ---\n");
    mlfq_test();

    // printf(1, "\n--- Exit Test ---\n");
    // exit_test();

    // printf(1, "\n--- Block Test ---\n");
    // block_test();

//     printf(1, "\n--- Yield Test ---\n");
//     yield_test();

//     printf(1, "\n--- Wakeup Test ---\n");
//     wakeup_test();

    // printf(1, "\n--- Time Slice Test ---\n");
    // priority_selection_test();

  printf(1, "====Finished Testing====\n");
  exit();
}
