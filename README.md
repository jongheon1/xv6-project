# xv6-virtual-memory

본 프로젝트는 [MIT xv6](https://pdos.csail.mit.edu/6.828/2022/xv6.html) 운영체제를 기반으로 한 프로젝트의 결과물입니다. xv6는 Unix v6를 x86 아키텍처에 맞게 재구현한 교육용 운영체제입니다.

원본 xv6 소스 코드의 저작권은 Frans Kaashoek, Robert Morris, Russ Cox가 가지고 있습니다.

## 구현 내용

본 프로젝트에서는 xv6 운영체제에 다음과 같은 기능을 추가로 구현했습니다:

1. **Priority Scheduler**
  - 프로세스의 우선순위(nice 값)에 따라
2. **MLFQ Scheduler**
  - 3개의 우선순위 큐(high, medium, low)를 사용하는 MLFQ(Multilevel Feedback Queue) Scheduler를 구현했습니다.

3. **Virtual Memory**
  - `mmap()`과 `munmap()` 시스템 콜을 구현하여 파일이나 디바이스를 프로세스의 가상 주소 공간에 매핑할 수 있도록 했습니다.
  - Demand Paging 기능을 구현하여 필요한 페이지만 메모리에 적재하도록 했습니다.
  - 스택과 힙 영역에 대한 Lazy Allocation을 적용했습니다.

 4. **Threads**

- `clone()` 시스템 콜을 구현하여 새로운 스레드를 생성하고, 부모 프로세스와 주소 공간을 공유할 수 있도록 했습니다.
- `join()` 시스템 콜을 구현하여 자식 스레드가 종료될 때까지 기다리고, 자원을 정리할 수 있도록 했습니다.
- `thread_create()`와 `thread_join()` API를 구현하여 스레드를 생성하고 관리할 수 있도록 했습니다.
- `mutex_lock()` 및 `mutex_unlock()` 시스템 콜을 구현하여 스레드 간의 동기화를 위한 상호 배제를 지원했습니다.

각 기능의 자세한 구현 내용과 테스트 방법은 해당 브랜치의 README 파일을 참조하세요.

## 브랜치 구성

- `main`: 프로젝트 개요 및 브랜치 설명
- `prio_sched`: Priority Scheduler 구현
- `mlfq_sched`: MLFQ Scheduler 구현
- `vm`: Virtual Memory 관련 기능 구현
- `thread`: 멀티스레드 관련 기능 구현

각 브랜치에서 구현된 기능을 확인하고 테스트해볼 수 있습니다.
