# 실습 4. `Thread` 단계별 구현 — pthread → Thread 클래스 → 가상 handler → android::Thread

> **소요시간:** 30분 · **난이도:** ★★☆ · **챕터:** Ch 1 libutils
> **디렉토리:** `~/android/day3/thread_test/thread.cpp`, `my_thread.cpp`

## 목표

- `pthread_create` 를 클래스로 감싸는 과정에서 **멤버 함수를 Thread 엔트리로 넘기는 규칙**(static 트램폴린 + `this`)을 익힌다.
- `android::Thread` 의 `run / readyToRun / threadLoop / join` 골격과, `sp` 에 담기는 순간 실행되는 `onFirstRef` 패턴을 이해한다.

---

## 단계 ① — pthread 를 클래스로 감싸기

```cpp
#include <stdio.h>
#include <pthread.h>

void* foo(void* data) { printf("child\n"); return 0; }

class Thread {
    pthread_t mThread;
public:
    void run()  { pthread_create(&mThread, 0, foo, 0); }
    void join() { pthread_join(mThread, 0); }
};

int main() {
    { Thread t; t.run(); t.join(); }
    return 0;
}
```

`foo` 가 전역 함수라 클래스 멤버(`mData`)를 못 만진다.

## 단계 ② — static 트램폴린으로 this 전달

```cpp
class Thread {
    pthread_t mThread;
    int mData;
public:
    void handler() { mData = 10; printf("child mData=%d\n", mData); }
    static void* __handler(void* data) {          // ★ pthread 는 C 함수 포인터만 받는다
        Thread* self = (Thread*)data;             //    → static 으로 받고 this 로 복원
        self->handler();
        return 0;
    }
    void run()  { pthread_create(&mThread, 0, __handler, this); }   // ★ this 를 4번째 인자로
    void join() { pthread_join(mThread, 0); }
};
```

멤버 함수는 숨은 `this` 인자가 있어 `void*(*)(void*)` 타입이 아니다. static 함수로 받아 `this` 를 복원하는 것이 C++ 에서 콜백을 다루는 표준 패턴이다 (실습 5 의 Looper 콜백, JNI 콜백도 같다).

## 단계 ③ — 순수 가상 handler 로 프레임워크화

```cpp
class Thread {
    pthread_t mThread;
public:
    virtual void handler() = 0;                   // ★ 파생 클래스가 채운다
    static void* __handler(void* data) { ((Thread*)data)->handler(); return 0; }
    void run()  { pthread_create(&mThread, 0, __handler, this); }
    void join() { pthread_join(mThread, 0); }
    virtual ~Thread() {}
};

class MyThread : public Thread {
public:
    void handler() override { printf("MyThread\n"); }
};

int main() {
    { MyThread t; t.run(); t.join(); }
    return 0;
}
```

"실행 골격은 부모가, 내용은 자식이" — `android::Thread` 가 이 형태다.

## 단계 ④ — android::Thread (`utils/Thread.h`)

**`thread_test/thread.cpp`**

```cpp
#include <stdio.h>
#include <unistd.h>
#include <utils/Thread.h>
using namespace android;

class MyThread : public Thread {
public:
    bool threadLoop() override {                  // ★ true 반환 → 다시 호출 (루프)
        sleep(1);
        printf("\t\tMyThread::threadLoop()\n");
        return true;
    }
    void onFirstRef() override {                  // ★ 첫 sp 가 붙는 순간
        run("MyThread");                          //    → Thread 시작
    }
};

int main() {
    sp<Thread> thread = new MyThread;             // ★ 이 줄에서 onFirstRef → run
    while (1) { sleep(1); printf("main()\n"); }
    return 0;
}
```

```text
main()
		MyThread::threadLoop()
main()
		MyThread::threadLoop()
...
```

**`thread_test/my_thread.cpp`** — 생명주기 전체

```cpp
#include <stdio.h>
#include <unistd.h>
#include <utils/Thread.h>
using namespace android;

class MyThread : public Thread {
public:
    MyThread() { printf("MyThread::MyThread()\n"); }
    void onFirstRef() override {
        run("my_thread");
        printf("MyThread::onFirstRef()\n");
    }
    status_t readyToRun() override {              // ★ threadLoop 전에 1회
        printf("MyThread::readyToRun()\n");
        return NO_ERROR;                          //    실패 반환 시 Thread 종료
    }
    bool threadLoop() override {
        static int n = 0;
        printf("MyThread::threadLoop() %d\n", n);
        sleep(1);
        return ++n < 3;                           // ★ 3번 후 false → 종료
    }
};

int main() {
    sp<Thread> p = new MyThread;
    p->join();                                    // ★ threadLoop 이 false 를 반환할 때까지 대기
    printf("parent\n");
    return 0;
}
```

```text
MyThread::MyThread()
MyThread::onFirstRef()
MyThread::readyToRun()
MyThread::threadLoop() 0
MyThread::threadLoop() 1
MyThread::threadLoop() 2
parent
```

## 빌드 · 실행

```bash
m thread_test my_thread_test
adb push $OUT/system/bin/thread_test $OUT/system/bin/my_thread_test /data
adb shell "chmod 755 /data/thread_test /data/my_thread_test; /data/my_thread_test"
```

## 실험 — Thread 가 RefBase 인 이유

`my_thread.cpp` 의 `main` 을 이렇게 바꿔 본다.

```cpp
int main() {
    { sp<Thread> p = new MyThread; }   // sp 가 바로 사라짐
    sleep(5);
    printf("parent\n");
}
```

그래도 `threadLoop` 이 3번 돈다. `Thread::run()` 이 내부에서 **자기 자신을 `sp<Thread>` 로 한 번 더 잡아** 실행 중에는 소멸되지 않게 한다 (`Thread::_threadLoop` 의 `sp<Thread> strong(self->mHoldSelf)`). `:tag Thread::_threadLoop` 로 확인.

---

## 확인 포인트

- [ ] ② `this` 없이 static 함수에서 멤버 접근이 안 됨을 확인
- [ ] ④ `sp<Thread> p = new MyThread;` 만으로 실행 시작
- [ ] `readyToRun` 1회 → `threadLoop` 반복 → `false` 로 종료 → `join` 반환
- [ ] `sp` 를 바로 버려도 Thread 가 끝까지 도는 것 (`mHoldSelf`)

## 핵심 정리

| android::Thread | 의미 |
|---|---|
| `run(name)` | pthread 생성. 이름은 `ps -T` 와 `binder_logs` 에 보인다 |
| `readyToRun()` | 첫 `threadLoop` 전 1회 초기화. `NO_ERROR` 외 반환 시 시작 취소 |
| `threadLoop()` | **true = 다시 호출, false = 종료**. 무한 루프를 직접 쓰지 않는다 |
| `requestExit()` / `exitPending()` | 밖에서 종료 요청 → 루프 안에서 확인 |
| `join()` | 종료 대기 |
| `onFirstRef()` | `RefBase` 훅 — `sp` 에 담기는 순간 `run` (생성자에서는 `sp<this>` 를 못 만드니까) |

Day 2 의 Binder Thread 도 이 `Thread` 다 — 실습 6 에서 `PoolThread::threadLoop` 을 연다.
