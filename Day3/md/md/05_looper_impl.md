# 실습 5. `Looper` 단계별 — pollOnce → wake → addFd 콜백 → 콜백 클래스 → MessageHandler

> **소요시간:** 40분 · **난이도:** ★★★ · **챕터:** Ch 1 libutils
> **디렉토리:** `~/android/day3/looper_test/Looper.cpp` (`#if` 로 단계 전환)

## 목표

- `Looper` 가 **fd 이벤트 루프(epoll)** 와 **메시지 큐** 두 얼굴을 가진 것을 단계별로 본다.
- 실습 4 의 `Thread` 가 다른 쪽에서 `wake` / `write` / `sendMessage` 를 보내는 생산자가 된다.
- 여기서 만든 `LooperCallback` 패턴이 실습 10 VSync 핸들러로 그대로 간다.

---

## 단계 ① — pollOnce 와 timeout

```cpp
#include <stdio.h>
#include <utils/Looper.h>
using namespace android;

int main() {
    sp<Looper> p = new Looper(true);          // true = allowNonCallbacks
    int result = p->pollOnce(1000);           // 1초 대기
    if (result == Looper::POLL_TIMEOUT) printf("POLL_TIMEOUT\n");
    return 0;
}
```

아무 일도 없으면 1초 후 `POLL_TIMEOUT`. 내부는 `epoll_wait(mEpollFd, …, timeout)`.

## 단계 ② — 다른 Thread 가 wake

```cpp
#include <stdio.h>
#include <unistd.h>
#include <utils/Looper.h>
#include <utils/Thread.h>
using namespace android;

class MyThread : public Thread {
    sp<Looper> mLooper;
public:
    MyThread(sp<Looper> looper) : mLooper(looper) {}
    bool threadLoop() override {
        sleep(3);
        mLooper->wake();                      // ★ eventfd 에 write → epoll 깨어남
        return false;
    }
};

int main() {
    sp<Looper> looper = new Looper(true);
    sp<Thread> p = new MyThread(looper);
    p->run("MyThread");

    int result = looper->pollOnce(-1);        // -1 = 무한 대기
    if (result == Looper::POLL_TIMEOUT) printf("POLL_TIMEOUT\n");
    if (result == Looper::POLL_WAKE)    printf("POLL_WAKE\n");
    return 0;
}
```

3초 후 `POLL_WAKE`. `Looper` 는 생성 시 `eventfd` 하나를 epoll 에 등록해 두고, `wake()` 는 거기에 1을 쓴다.

## 단계 ③ — addFd 와 콜백 함수

```cpp
class MyThread : public Thread {
    sp<Looper> mLooper; int mFd;
public:
    MyThread(sp<Looper> looper, int fd) : mLooper(looper), mFd(fd) {}
    bool threadLoop() override { sleep(3); write(mFd, "W", 1); return false; }
};

int foo(int fd, int events, void* data) {     // ★ Looper_callbackFunc 시그니처
    printf("foo(%d, %d, %p)\n", fd, events, data);
    char c; read(fd, &c, 1);                  //    읽어 줘야 다시 안 깨어남
    return 0;                                 //    0 = 이 fd 콜백 제거, 1 = 유지
}

int main() {
    int fd[2]; pipe(fd);
    sp<Looper> looper = new Looper(true);
    sp<Thread> p = new MyThread(looper, fd[1]);

    looper->addFd(fd[0], 0, Looper::EVENT_INPUT, foo, (void*)0x1234);   // ★ fd 를 epoll 에 등록
    p->run("MyThread");

    while (1) {
        int result = looper->pollOnce(-1);
        if (result == Looper::POLL_TIMEOUT)  printf("POLL_TIMEOUT\n");
        if (result == Looper::POLL_WAKE)     printf("POLL_WAKE\n");
        if (result == Looper::POLL_CALLBACK) printf("POLL_CALLBACK\n");
    }
    return 0;
}
```

```text
foo(3, 1, 0x1234)
POLL_CALLBACK
```

`data` 로 넘긴 `0x1234` 가 그대로 돌아온다 — 실습 4 ② 의 `this` 전달 자리가 여기다.

## 단계 ④ — 콜백을 클래스로 (LooperCallback 의 정체)

```cpp
class MyHandler : public RefBase {
    static int staticHandler(int fd, int events, void* data) {   // ★ 트램폴린
        return ((MyHandler*)data)->handler(fd, events);
    }
public:
    virtual int handler(int fd, int events) = 0;                 // ★ 파생이 채운다
    void setCallback(sp<Looper>& looper, int fd, int events) {
        looper->addFd(fd, 0, events, staticHandler, this);       // ★ this 를 data 로
    }
};

class StubMyHandler : public MyHandler {
public:
    int handler(int fd, int events) override {
        printf("StubMyHandler::handler(%d, %d)\n", fd, events);
        char c; read(fd, &c, 1);
        return 1;
    }
};

int main() {
    int fd[2]; pipe(fd);
    sp<Looper> looper = new Looper(true);
    sp<Thread> p = new MyThread(looper, fd[1]);
    sp<MyHandler> handler = new StubMyHandler;
    handler->setCallback(looper, fd[0], Looper::EVENT_INPUT);
    p->run("MyThread");
    while (1) { looper->pollOnce(-1); }
}
```

Android 는 이것을 이미 제공한다 — `LooperCallback` (`utils/Looper.h`):

```cpp
class LooperCallback : public virtual RefBase {
public:
    virtual int handleEvent(int fd, int events, void* data) = 0;
};
looper->addFd(fd, 0, events, new MyCallback(), nullptr);   // sp<LooperCallback> 오버로드
```

실습 10 의 `VSyncHandler : LooperCallback` 이 정확히 이 형태다.

## 단계 ⑤ — 메시지 큐: sendMessage / sendMessageDelayed

```cpp
class MyMessageHandler : public MessageHandler {
public:
    void handleMessage(const Message& message) override {
        printf("MyMessageHandler::handleMessage(%d)\n", message.what);
    }
};

class MyThread : public Thread {
    sp<Looper> mLooper; sp<MessageHandler> mHandler;
public:
    MyThread(sp<Looper> l, sp<MessageHandler> h) : mLooper(l), mHandler(h) {}
    bool threadLoop() override {
        sleep(3);
        mLooper->sendMessage(mHandler, Message(1));
        mLooper->sendMessage(mHandler, Message(2));
        mLooper->sendMessageDelayed(3 * 1000 * 1000 * 1000LL, mHandler, Message(3));   // ns
        printf("after\n");
        return false;
    }
};

int main() {
    sp<Looper> looper = new Looper(true);
    sp<MessageHandler> handler = new MyMessageHandler();
    sp<Thread> p = new MyThread(looper, handler);
    p->run("MyThread");
    while (1) {
        int r = looper->pollOnce(-1);
        if (r == Looper::POLL_WAKE) printf("POLL_WAKE\n");
        if (r == Looper::POLL_CALLBACK) printf("POLL_CALLBACK\n");
    }
}
```

```text
after
MyMessageHandler::handleMessage(1)
MyMessageHandler::handleMessage(2)
POLL_CALLBACK
(3초 후)
MyMessageHandler::handleMessage(3)
POLL_CALLBACK
```

`sendMessage` 는 메시지를 큐(`mMessageEnvelopes`)에 넣고 `wake()` 한다. `pollOnce` 는 fd 이벤트를 처리한 뒤 만기된 메시지를 `handleMessage` 로 보낸다. Java `Handler.sendMessageDelayed` 의 실체다.

## 빌드 · 실행

```bash
m looper_test
adb push $OUT/system/bin/looper_test /data && adb shell chmod 755 /data/looper_test && adb shell /data/looper_test
```

---

## 확인 포인트

- [ ] ① `POLL_TIMEOUT`, ② 3초 후 `POLL_WAKE`, ③ `foo(...)` + `POLL_CALLBACK`
- [ ] ④ `this` 를 `data` 로 넘겨 가상 함수가 호출됨
- [ ] ⑤ 1, 2 는 즉시, 3 은 3초 후 — 큐 순서와 지연
- [ ] `strace -e epoll_wait,epoll_ctl,eventfd2 /data/looper_test` 로 내부 시스템 콜 확인

## 핵심 정리

| 단계 | Looper 기능 | 시스템 콜 |
|---|---|---|
| ① | `pollOnce(timeout)` | `epoll_wait` |
| ② | `wake()` | `write(eventfd)` |
| ③ ④ | `addFd(fd, …, callback, data)` — **fd 이벤트 루프** | `epoll_ctl(ADD)` |
| ⑤ | `sendMessage[Delayed]` + `MessageHandler` — **메시지 큐** | 큐 + `wake` |

Java `Looper / Handler / MessageQueue` 는 이 C++ Looper 의 JNI wrapper (`android_os_MessageQueue.cpp` 의 `nativePollOnce` = `Looper::pollOnce`). 실습 6·8 에서 소스로 확인한다.
