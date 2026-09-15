# 실습 2. 최소 Native Binder Server — BBinder 하나 등록하기

> **소요시간:** 20분 · **난이도:** ★☆☆ · **챕터:** Ch 1 Native Binder
> **디렉토리:** `~/aosp/day2/binder1`

## 목표

- libbinder 의 세 축 `ProcessState` · `IServiceManager` · `IPCThreadState` 를 처음 만난다.
- 아무 기능도 없는 `BBinder` 하나를 ServiceManager 에 등록하고 `service list` 에서 확인한다.

## 원리

| 클래스 | 역할 |
|---|---|
| `ProcessState::self()` | Process 당 하나. `/dev/binder` open + mmap, Thread Pool 관리 |
| `defaultServiceManager()` | handle 0 → ServiceManager Proxy |
| `BBinder` | Server 측 Binder 객체의 기본 클래스 (`onTransact` 를 가진다) |
| `IPCThreadState::self()->joinThreadPool()` | 현재 Thread 를 Binder Thread 로 만들어 요청 대기 |

---

## Step 1. 소스 작성

**`binder1/my_server.cpp`**

```cpp
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>

using namespace android;

int main()
{
    sp<ProcessState> proc(ProcessState::self());          // /dev/binder open, mmap
    sp<IServiceManager> sm = defaultServiceManager();     // handle 0

    status_t st = sm->addService(String16("my.service"), new BBinder);   // ★ 기능 없는 Binder 객체
    printf("addService(\"my.service\") = %d, pid=%d\n", st, getpid());
    fflush(stdout);

    IPCThreadState::self()->joinThreadPool();             // ★ 이 줄에서 영원히 대기
    return 0;
}
```

## Step 2. 빌드 · 배포

```bash
cd ~/aosp/day2 && mm -j$(nproc)          # 또는 m my_server_test_1
ls $OUT/system/bin/my_server_test_1
```

WinSCP 로 받아서:

```bat
adb root
adb push my_server_test_1 /data
adb shell chmod 755 /data/my_server_test_1
```

## Step 3. 실행 · 확인

터미널 ①:

```bash
adb shell /data/my_server_test_1
# addService("my.service") = 0, pid=5321
```

터미널 ②:

```bash
adb shell
service list | grep my.service
# 130  my.service: []                    ← descriptor 가 비어 있다 (인터페이스 없는 raw BBinder)

SPID=$(pidof my_server_test_1)
cat /dev/binderfs/binder_logs/proc/$SPID | grep -E "thread|node"
# thread 5321: l 00 need_return 1 tr 0     ← main thread 가 Binder Thread 로 합류
# node 17: u0000... c0000... pri 0:120 hs 1 hw 1 ls 0 lw 0 is 1 iw 1 ...   ← 우리 BBinder 의 node

# Transaction 을 던져 보면?
service call my.service 1
# Result: Parcel(ffffffffffffffff -1)   ← BBinder::onTransact 기본 구현 = UNKNOWN_TRANSACTION
```

`Ctrl+C` 로 서버를 죽이면 `service list` 에서 사라진다 (ServiceManager 가 DeathRecipient 로 감지).

---

## 확인 포인트

- [ ] `service list` 에 `my.service` 표시
- [ ] `binder_logs/proc/<PID>` 에 node 1개, thread 1개
- [ ] `service call my.service 1` → `-1` (UNKNOWN_TRANSACTION)
- [ ] 서버 종료 후 `service list` 에서 사라짐

## 핵심 정리

- Native Binder Server 의 최소 형태 = `ProcessState` + `addService` + `joinThreadPool`.
- `BBinder` 는 `onTransact()` 를 오버라이드하지 않으면 모든 code 에 `UNKNOWN_TRANSACTION` 을 돌려준다. 다음 실습에서 이것을 채운다.
- `joinThreadPool()` 은 반환하지 않는다 — 서버 main 의 마지막 줄이다.

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `addService = -1` | SELinux — `adb shell setenforce 0` (커스텀 이미지는 이미 permissive) |
| `mm` 이 다시 6분 | `Android.bp` 를 건드렸는지 확인. `.cpp` 만 고쳤다면 분석이 다시 돌지 않는다 |
