# xv6-projects

본 프로젝트는 [MIT xv6](https://pdos.csail.mit.edu/6.828/2022/xv6.html) 운영체제를 기반으로 한 프로젝트의 결과물입니다. xv6는 Unix v6를 x86 아키텍처에 맞게 재구현한 교육용 운영체제입니다.

각 프로젝트는 xv6 운영체제에 새로운 기능을 추가하거나 기존 기능을 개선하는 것을 목표로 합니다. 이를 통해 운영체제의 다양한 구성 요소와 그 상호작용에 대한 이해도를 높일 수 있었습니다.

원본 xv6 소스 코드의 저작권은 Frans Kaashoek, Robert Morris, Russ Cox가 가지고 있습니다.

## 프로젝트 개요

### Priority Scheduler
이 프로젝트에서는 프로세스의 우선순위(nice 값)에 따라 CPU 자원을 할당하는 Priority Scheduler를 구현했습니다. 

우선순위가 높은 프로세스에 더 많은 CPU 시간을 할당함으로써, 시스템의 응답성을 개선할 수 있습니다.

자세한 내용은 prio_sched 브랜치의 README 파일을 참조하세요.

### MLFQ Scheduler
이 프로젝트에서는 3개의 우선순위 큐(high, medium, low)를 사용하는 MLFQ(Multilevel Feedback Queue) Scheduler를 구현했습니다.

프로세스의 CPU 사용 패턴에 따라 우선순위를 동적으로 조정함으로써, 시스템 전체의 성능과 공정성을 향상시킬 수 있습니다.

자세한 내용은 mlfq_sched 브랜치의 README 파일을 참조하세요.

### Virtual Memory
이 프로젝트에서는 가상 메모리 관련 기능을 구현했습니다. 

mmap()과 munmap() 시스템 콜을 통해 파일이나 디바이스를 프로세스의 가상 주소 공간에 매핑할 수 있도록 했습니다.

또한 Demand Paging 기법을 도입하여 필요한 페이지만 메모리에 적재하도록 했으며, 스택과 힙 영역에 대한 Lazy Allocation을 적용했습니다.

자세한 내용은 vm 브랜치의 README 파일을 참조하세요.

### Threads
이 프로젝트에서는 멀티스레드 관련 기능을 구현했습니다.

clone() 시스템 콜을 통해 새로운 스레드를 생성하고, 부모 프로세스와 주소 공간을 공유할 수 있도록 했습니다. 또한 join() 시스템 콜을 구현하여 자식 스레드가 종료될 때까지 기다리고, 자원을 정리할 수 있도록 했습니다. 

사용자 레벨에서는 thread_create()와 thread_join() API를 제공하여 스레드를 쉽게 생성하고 관리할 수 있도록 했습니다.

아울러 mutex_lock()과 mutex_unlock() 시스템 콜을 구현하여 스레드 간의 동기화를 위한 상호 배제를 지원했습니다.

자세한 내용은 thread 브랜치의 README 파일을 참조하세요.

## 브랜치 구성

- `main`: 프로젝트 개요 및 브랜치 설명
- `prio_sched`: Priority Scheduler 구현
- `mlfq_sched`: MLFQ Scheduler 구현
- `vm`: Virtual Memory 관련 기능 구현
- `thread`: 멀티스레드 관련 기능 구현

각 브랜치에서 구현된 기능을 확인하고 테스트해볼 수 있습니다.

## 프로젝트의 의의
이 일련의 프로젝트들은 운영체제의 핵심 개념과 구조에 대한 깊이 있는 이해를 도모하고, 실제 구현을 통해 그 지식을 내재화하는 경험을 제공합니다. 스케줄링, 가상 메모리, 멀티스레딩 등 운영체제의 주요 구성 요소를 직접 다뤄보면서, 시스템 프로그래밍에 대한 감각을 키울 수 있었습니다.

특히, xv6라는 교육용 운영체제를 기반으로 작업함으로써, 복잡도를 낮추면서도 실제 운영체제의 동작 원리를 충실히 반영할 수 있었습니다. 이는 운영체제 내부 구조에 대한 통찰력을 기르고, 새로운 아이디어를 실험해 볼 수 있는 발판이 되었습니다.

이러한 경험은 소프트웨어 개발자로서의 역량을 한 단계 성장시키는 데 큰 도움이 될 것입니다. 운영체제는 모든 소프트웨어의 토대가 되는 만큼, 이에 대한 깊은 이해는 효율적이고 신뢰성 높은 소프트웨어를 개발하는 데 필수적입니다.
앞으로도 운영체제와 시스템 프로그래밍 분야에 깊은 관심을 갖고, 계속해서 도전적인 프로젝트에 매진할 것입니다. 이번 프로젝트들이 그 여정에 있어 소중한 이정표가 되기를 기대합니다.
