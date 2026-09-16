# 실습 6. Binder 에서의 동작 — PoolThread · joinThreadPool · Handler 로 Main Thread 전달

> **소요시간:** 20분 · **난이도:** ★★☆ · **챕터:** Ch 1 libutils ↔ Day 2 Binder
> **환경:** 빌드 서버 (vim + tags) + Emulator (Day 2 `my_server_test_4` 재사용)

## 목표

- Day 2 의 Binder Thread 가 실습 4 의 `android::Thread` 임을 소스에서 확인한다.
- Binder Thread 에는 Looper 가 없다 → 콜백을 Main Thread 로 보내는 것이 `Handler.post` = `Looper.sendMessage` 임을 잇는다.
- `joinThreadPool` 의 대기 루프와 `Looper::pollOnce` 의 대기 루프를 비교한다.

---

## Step 1. Binder Thread Pool 의 정체 (소스 읽기)

```bash
cd ~/android/frameworks/native/libs/binder
vim ProcessState.cpp
:tag ProcessState::spawnPooledThread
```

```cpp
class PoolThread : public Thread {                        // ★ 실습 4 의 android::Thread
public:
    explicit PoolThread(bool isMain) : mIsMain(isMain) {}
protected:
    virtual bool threadLoop() {
        IPCThreadState::self()->joinThreadPool(mIsMain);  // ★ 반환하지 않는 대기 루프
        return false;
    }
    const bool mIsMain;
};

void ProcessState::spawnPooledThread(bool isMain) {
    if (mThreadPoolStarted) {
        String8 name = makeBinderThreadName();            // "binder:<pid>_<n>"
        sp<Thread> t = sp<PoolThread>::make(isMain);
        t->run(name.c_str());                             // ★ Thread::run
        ...
    }
}
```

`startThreadPool()` → `spawnPooledThread(true)` 로 첫 Thread, 이후 Driver 가 `BR_SPAWN_LOOPER` 를 보낼 때마다 `spawnPooledThread(false)`.

## Step 2. joinThreadPool 의 루프

```bash
:tag IPCThreadState::joinThreadPool
```

```cpp
void IPCThreadState::joinThreadPool(bool isMain) {
    mOut.writeInt32(isMain ? BC_ENTER_LOOPER : BC_REGISTER_LOOPER);
    do {
        result = getAndExecuteCommand();      // ★ talkWithDriver() → ioctl(BINDER_WRITE_READ) → executeCommand
    } while (result != -ECONNREFUSED && result != -EBADF);
    mOut.writeInt32(BC_EXIT_LOOPER);
    talkWithDriver(false);
}
```

| | `IPCThreadState::joinThreadPool` | `Looper::pollOnce` |
|---|---|---|
| 대기 대상 | Binder Driver (`ioctl` 블로킹) | fd 집합 (`epoll_wait`) |
| 깨우는 것 | 다른 Process 의 `transact` | fd 이벤트 · `wake()` · 메시지 만기 |
| 실행되는 것 | `BBinder::onTransact` | `LooperCallback::handleEvent` · `MessageHandler::handleMessage` |
| 결론 | Binder Thread 에는 **Looper 가 없다** — UI · 메시지 큐 접근 불가 |

## Step 3. startThreadPool 유무 실험 (Emulator)

Day 2 `my_server_test_4` 를 그대로 쓴다.

```bash
adb shell /data/my_server_test_4 &
SPID=$(adb shell pidof my_server_test_4)
adb shell "ps -T -p $SPID"                      # binder:<pid>_1, _2 … Thread 이름 = Thread::run(name)
adb shell "cat /dev/binderfs/binder_logs/proc/$SPID" | grep -c "^  thread"

# 동시 호출 3개 → 서버 로그의 tid 가 달라진다 (Thread Pool)
for i in 1 2 3; do adb shell /data/my_client_test_4 $i & done; wait
```

`my_server.cpp` 에서 `startThreadPool()` 줄을 주석 처리하고 다시 빌드·실행하면 main 하나만 `joinThreadPool` 에 있어 tid 가 모두 같고 순차 처리된다.

## Step 4. Java 콜백은 어떻게 Main Thread 로 가는가

Day 1 실습 9 `StockLab` 의 콜백:

```java
public void onPriceUpdate(...) {            // Binder Thread 에서 호출 (Looper 없음)
    runOnUiThread(() -> ...);               // = new Handler(Looper.getMainLooper()).post(r)
}
```

`Handler.post` → `MessageQueue.enqueueMessage` → `nativeWake` → C++ `Looper::wake` — 실습 5 ② 그대로다.

```bash
cd ~/android/frameworks/base/core/jni
vim android_os_MessageQueue.cpp
:tag android_os_MessageQueue_nativePollOnce      # → mLooper->pollOnce(timeoutMillis)
:tag NativeMessageQueue::wake                     # → mLooper->wake()
```

Java `Looper` 하나 = C++ `Looper` 하나. Main Thread 의 `Looper.loop()` 는 `nativePollOnce` 를 반복 호출하는 것이다.

---

## 확인 포인트

- [ ] `PoolThread : Thread` 와 `threadLoop() { joinThreadPool }` 을 소스에서 찾음
- [ ] `ps -T` 에서 `binder:<pid>_N` Thread 이름 확인
- [ ] `startThreadPool` 유무로 tid 분산/동일 비교
- [ ] `nativePollOnce` → `Looper::pollOnce` 확인

## 핵심 정리

- Binder Thread = `android::Thread` + `joinThreadPool` (Driver 대기 루프).
- Main Thread = `Looper::pollOnce` (fd + 메시지 큐 대기 루프).
- 둘은 다른 루프라 서로의 일을 직접 못 한다 → Binder 콜백에서 UI 는 `Handler.post` 로 **Looper 큐에 넣어** 넘긴다.
- Day 2 실습 6 의 Client `joinThreadPool` 과 실습 10 VSync 의 `pollAll` 이 각각 이 두 루프다.
