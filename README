# xv6-virtual-memory

이 프로젝트는 MIT의 xv6 운영체제를 기반으로 가상 메모리 기능을 추가 구현한 것입니다. 

## 원 프로젝트 정보

이 프로젝트는 [MIT xv6](https://pdos.csail.mit.edu/6.828/2022/xv6.html) 운영체제를 기반으로 합니다. xv6는 Unix v6를 x86 아키텍처에 맞게 재구현한 교육용 운영체제입니다. 

xv6의 소스 코드는 Frans Kaashoek, Robert Morris, Russ Cox가 저작권을 갖고 있습니다. 자세한 내용은 [GitHub 저장소](https://github.com/mit-pdos/xv6-riscv)를 참조하세요.

# Priority Scheduler

이 브랜치에서는 xv6 운영체제에 Priority Scheduler를 구현했습니다.

## 구현 내용

### 스케줄러 수정

`scheduler()` 함수를 수정하여 프로세스의 우선순위(nice 값)에 따라 스케줄링하도록 변경했습니다. 우선순위가 높은 프로세스를 먼저 실행하고, 우선순위가 같은 경우 PID가 낮은 프로세스를 먼저 실행합니다.

### 스케줄링 포인트 처리

다음과 같은 시점에서 스케줄링이 발생하도록 수정했습니다:
- 프로세스 종료 시 (`exit()`)
- 프로세스 블록 시 (`sleep()`, `read()` 등)
- `yield()` 시스템 콜 호출 시
- 프로세스의 우선순위 변경 시 (`nice()`)
- 프로세스 wake up 시 (`wakeup()`)

이를 통해 모든 스케줄링 결과가 Priority Scheduling 정책을 따르도록 했습니다.
