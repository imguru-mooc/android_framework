# 실습 6. Binder 탐정 — System Service 추적하기

> **소요시간:** 30분 · **난이도:** ★★☆ · **챕터:** Ch 1 Binder
> **환경:** Emulator (커스텀 이미지, adb root) + Ubuntu 빌드 서버 (Step 4 는 `mm` 빌드 필요)

## 목표

- App이 System Service를 호출할 때 Binder Transaction이 발생하는 것을 실시간 관찰한다.
- `service call` 로 System Service를 직접 호출한다.
- **직접 만든 Native Binder Server를 띄우고 Client가 접속하는 과정을 ftrace에서 추적한다.**

---

## Step 1. `service call` 로 직접 Service 호출

```bash
adb root
adb shell
```

```bash
# clipboard Service 호출 (IClipboard transaction code 1)
service call clipboard 1

dumpsys display | grep "mBaseDisplayInfo"
dumpsys battery
```

`service call <name> <code>` 는 AIDL Method 순서(1부터)를 그대로 Transaction code로 사용한다. Proxy 없이도 Binder Driver를 통해 호출된다는 것을 보여 준다.

## Step 2. Binder Transaction 실시간 관찰 (ftrace)

```bash
# 1. 이벤트 활성화 확인 (1 이어야 함)
cat /sys/kernel/tracing/events/binder/binder_transaction/enable
echo 1 > /sys/kernel/tracing/events/binder/binder_transaction/enable

# 2. tracing_on 확인
cat /sys/kernel/tracing/tracing_on
echo 1 > /sys/kernel/tracing/tracing_on

# 3. 실시간 출력
cat /sys/kernel/tracing/trace_pipe
```

Emulator에서 앱을 열거나 설정을 바꾸면 Transaction 로그가 흘러나온다.

```text
 com.android.car-1832  [001] ...  binder_transaction: transaction=12345 dest_node=... dest_proc=612 dest_thread=0 reply=0 flags=0x10 code=0x2
```

| 필드 | 의미 |
|---|---|
| `dest_proc` | 수신 Process PID (system_server 등) |
| `reply` | 0 = 요청, 1 = 응답 |
| `flags & 0x1` | oneway |
| `code` | Transaction code |

```bash
# 멈추기 / 비활성화
echo 0 > /sys/kernel/tracing/tracing_on
echo 0 > /sys/kernel/tracing/events/binder/binder_transaction/enable
```

## Step 3. 특정 App의 Binder 사용량 추적

```bash
SETPID=$(pidof com.android.car.settings)

cat /dev/binderfs/binder_logs/proc/$SETPID

# Binder Thread 수
cat /dev/binderfs/binder_logs/proc/$SETPID | grep "thread"
```

---

## Step 4. 직접 만든 Binder Server ↔ Client 를 trace 로 보기

지금까지는 이미 떠 있는 System Service 를 관찰했다. 이번에는 **우리가 만든 Binder Server** 를 띄우고, Client 가 접속해서 Transaction 을 보내는 순간을 ftrace 로 잡는다. AIDL 없이 `BBinder::onTransact()` 를 직접 구현해 Binder 의 실체(code + Parcel)를 그대로 본다.

### 4-1. 소스 작성 (빌드 서버)

```bash
cd ~/aosp
source build/envsetup.sh
lunch sdk_car_x86_64-aosp_current-userdebug
mkdir -p ~/aosp/hello_binder && cd ~/aosp/hello_binder
```

**`hello_server.cpp`**

```cpp
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/Parcel.h>
#include <cstdio>
#include <unistd.h>

using namespace android;

// ★ AIDL 없이 Binder 객체를 직접 구현 — Stub 이 하는 일을 손으로 쓴다
class HelloService : public BBinder {
protected:
    status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) override {
        IPCThreadState* ipc = IPCThreadState::self();
        printf("[server] onTransact code=%u from pid=%d uid=%d  (server tid=%d)\n",
               code, ipc->getCallingPid(), ipc->getCallingUid(), gettid());
        fflush(stdout);

        switch (code) {
        case 1: {                                   // add(a, b)
            int32_t a = data.readInt32();
            int32_t b = data.readInt32();
            reply->writeInt32(a + b);
            printf("[server]   add(%d, %d) = %d\n", a, b, a + b);
            fflush(stdout);
            return NO_ERROR;
        }
        default:
            return BBinder::onTransact(code, data, reply, flags);
        }
    }
};

int main() {
    sp<ProcessState> ps = ProcessState::self();     // /dev/binder open + mmap
    sp<IServiceManager> sm = defaultServiceManager();

    status_t st = sm->addService(String16("hello.binder"), new HelloService());
    printf("[server] addService(\"hello.binder\") = %d, pid=%d\n", st, getpid());
    fflush(stdout);

    ps->startThreadPool();                          // Binder Thread Pool 시작
    IPCThreadState::self()->joinThreadPool();       // main thread 도 Binder Thread 로 합류
    return 0;
}
```

**`hello_client.cpp`**

```cpp
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

using namespace android;

int main(int argc, char** argv) {
    int32_t a = argc > 1 ? atoi(argv[1]) : 40;
    int32_t b = argc > 2 ? atoi(argv[2]) : 2;

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->checkService(String16("hello.binder"));   // ServiceManager 조회
    if (binder == nullptr) {
        printf("[client] hello.binder not found — server 가 떠 있나?\n");
        return 1;
    }

    Parcel data, reply;
    data.writeInt32(a);
    data.writeInt32(b);
    printf("[client] pid=%d transact(code=1, %d, %d) ...\n", getpid(), a, b);
    fflush(stdout);

    status_t st = binder->transact(1, data, &reply);      // ★ ioctl(BINDER_WRITE_READ)
    printf("[client] status=%d reply=%d\n", st, reply.readInt32());
    return 0;
}
```

**`Android.bp`**

```text
cc_binary {
    name: "hello_server",
    srcs: ["hello_server.cpp"],
    shared_libs: ["libbinder", "libutils", "liblog"],
    cflags: ["-Wall", "-Werror"],
}

cc_binary {
    name: "hello_client",
    srcs: ["hello_client.cpp"],
    shared_libs: ["libbinder", "libutils", "liblog"],
    cflags: ["-Wall", "-Werror"],
}
```

```bash
mm -j$(nproc)
ls $OUT/system/bin/hello_server $OUT/system/bin/hello_client
```

### 4-2. Emulator 에 배포 (Windows)

WinSCP 로 두 바이너리를 받은 뒤:

```bat
adb root
adb push hello_server /data
adb push hello_client /data
adb shell chmod 755 /data/hello_server /data/hello_client
```

### 4-3. Server 띄우기 — 터미널 ①

```bash
adb shell
```

```bash
setenforce 0                      # addService 는 SELinux 정책 검사를 받는다 (커스텀 이미지는 이미 permissive)
/data/hello_server
# [server] addService("hello.binder") = 0, pid=5123
```

이 창은 그대로 둔다. 서버는 `joinThreadPool()` 에서 대기 중이다.

### 4-4. 등록 확인 + trace 준비 — 터미널 ②

```bash
adb shell
```

```bash
service list | grep hello
# 123  hello.binder: []              ← descriptor 없음 = AIDL 없이 만든 raw Binder
```

이제 **서버가 받은 것과 보낸 것만** 남기는 커널 필터를 건다. 아래를 순서대로 실행한다.

```bash
# 0. 서버 PID
SPID=$(pidof hello_server)
[ -z "$SPID" ] && SPID=$(pgrep -f hello_server)
echo "server pid = $SPID"

# 1. 서버 스레드 tid 목록 → 발신자 조건 (common_pid 는 tid 기준)
TIDS=$(ls /proc/$SPID/task | sed 's/^/common_pid == /' | paste -sd'|' | sed 's/|/ || /g')

# 2. trace 초기화
echo 0 > /sys/kernel/tracing/tracing_on
echo > /sys/kernel/tracing/trace
echo 0 > /sys/kernel/tracing/events/binder/binder_transaction_received/enable   # 필드가 debug_id 뿐 → 사용 안 함

# 3. 필터: 서버로 들어오는 것 || 서버 스레드가 보내는 것
echo "to_proc == $SPID || $TIDS" > /sys/kernel/tracing/events/binder/binder_transaction/filter
cat /sys/kernel/tracing/events/binder/binder_transaction/filter     # parse_error 없이 조건이 보여야 함

# 4. 시작
echo 1 > /sys/kernel/tracing/events/binder/binder_transaction/enable
echo 1 > /sys/kernel/tracing/tracing_on
cat /sys/kernel/tracing/trace_pipe
```

**필드 이름 주의** — trace 출력에는 `dest_proc`, `dest_thread` 로 찍히지만 필터에 쓰는 실제 필드명은 `to_proc`, `to_thread` 다. `cat /sys/kernel/tracing/events/binder/binder_transaction/format` 의 `print fmt` 줄에서 대응 관계를 확인할 수 있다.

```text
field:int to_proc;   field:int to_thread;   field:int target_node;   field:int reply;   field:unsigned int code;
print fmt: "... dest_proc=%d dest_thread=%d ...", REC->to_proc, REC->to_thread, ...
```

> **왜 `binder_transaction_received` 는 끄는가** — 이 이벤트는 필드가 `debug_id` 하나뿐이라 `to_proc` 조건을 걸 수 없고, 필터 없이 켜면 Emulator 가 가만히 있어도 Automotive 서비스 간 oneway 콜백(`binder:668_5`, `com.android.car` 등)이 초당 수십 건 잡힌다. 응답 줄의 task 이름(`binder:<SPID>_N`)으로 어느 스레드가 받았는지 이미 알 수 있으므로 필요 없다. 굳이 보려면 `echo "$TIDS" > …/binder_transaction_received/filter` 후 enable 한다.

### 4-5. Client 접속 — 터미널 ③

```bash
adb shell /data/hello_client 40 2
```

```text
[client] pid=5210 transact(code=1, 40, 2) ...
[client] status=0 reply=42
```

동시에 **터미널 ①** (server) 에는:

```text
[server] onTransact code=1 from pid=5210 uid=0  (server tid=5125)
[server]   add(40, 2) = 42
```

**터미널 ②** (trace) 에는:

```text
 hello_client-5210   [000] ....  binder_transaction: transaction=48213 dest_node=17 dest_proc=5123 dest_thread=0 reply=0 flags=0x10 code=0x1
 binder:5123_1-5125  [001] ....  binder_transaction: transaction=48214 dest_node=0 dest_proc=5210 dest_thread=5210 reply=1 flags=0x0 code=0x0
```

| 줄 | 의미 |
|---|---|
| 1 | Client(5210) → Server(5123) 로 **code=0x1** 요청. `dest_thread=0` = Driver 가 대기 Thread 중 하나를 고른다 |
| 2 | Server 의 Binder Thread **binder:5123_1 (tid 5125)** 가 처리 후 **reply=1** 응답. `dest_thread=5210` = 기다리던 바로 그 Client Thread 로. task 이름이 server 로그의 `server tid` 와 일치 |

### 4-6. 변형 실험

```bash
# ① service call 로 Proxy 없이 같은 Transaction 보내기
service call hello.binder 1 i32 100 i32 23
# Result: Parcel(00000000 0000007b '........')   ← 0x7b = 123

# ② 여러 Client 동시 호출 → server tid 가 달라지는지 관찰 (Thread Pool)
#    Driver 가 새 Binder Thread 를 만들면 그 tid 는 필터에 없어 reply 가 빠질 수 있다 → 아래 한 줄로 필터 갱신 후 다시 실행
for i in 1 2 3; do /data/hello_client $i 1 & done; wait
TIDS=$(ls /proc/$SPID/task | sed 's/^/common_pid == /' | paste -sd'|' | sed 's/|/ || /g'); echo "to_proc == $SPID || $TIDS" > /sys/kernel/tracing/events/binder/binder_transaction/filter

# ③ server 를 죽이고 client 실행 → checkService 가 nullptr
kill $SPID
/data/hello_client
# [client] hello.binder not found — server 가 떠 있나?
```

### 4-7. 정리

```bash
echo 0 > /sys/kernel/tracing/tracing_on
echo 0 > /sys/kernel/tracing/events/binder/binder_transaction/filter
echo 0 > /sys/kernel/tracing/events/binder/binder_transaction/enable
```

**✅ 확인 포인트**

- `service list` 에 `hello.binder` 가 나타난다
- trace 요청 줄의 `dest_proc` = server PID, `code=0x1` = 우리가 정한 Transaction code
- 응답 줄은 `reply=1`, `dest_thread` = Client PID
- server 로그의 `from pid` = client PID, `uid=0` (adb root)
- 동시 호출 시 server tid 가 바뀐다 → Binder Thread Pool

---

## 🎯 결과물

- Settings 앱을 열 때 발생하는 Binder Transaction 수
- `dest_proc` 을 PID로 역추적해 어떤 Service가 호출되는지 추정
- **hello_client → hello_server 요청/응답 trace 2줄 캡처와 각 필드 해석**

## 핵심 정리

- App ↔ system_server 통신은 예외 없이 Binder Driver를 거친다.
- `binderfs/binder_logs` 와 ftrace `binder_transaction` 은 Binder 병목 분석의 기본 도구다.
- Binder 의 실체는 **code + Parcel** 이다. AIDL 의 Stub/Proxy 는 이것을 감싸는 편의 계층일 뿐이다 (실습 7 에서 확인).
- Day 2 의 `led_auth_service` 는 이 Step 4 를 AIDL cpp backend 로 다시 만드는 것이다.

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `addService = -1` (PERMISSION_DENIED) | `setenforce 0` 후 재실행, 또는 `adb root` 확인 |
| `hello.binder not found` | server 창이 종료됨 — 터미널 ① 을 다시 확인 |
| trace 에 아무것도 안 찍힘 | `tracing_on` 이 1 인지, `SPID` 가 비어 있지 않은지 (`echo $SPID`) 확인. `pidof` 가 빈 값이면 `pgrep -f hello_server` |
| 다른 Process 의 Transaction 까지 전부 나옴 | `cat …/binder_transaction/filter` 확인 — `none` 이면 `SPID` 가 비어 echo 실패, `parse_error: Field not found` 면 필드명 오류 (`dest_proc` ✕ → `to_proc` ○). `binder_transaction_received` 가 켜져 있으면 끈다 |
| reply 줄이 안 나옴 | 서버에 새 Binder Thread 가 생겨 tid 가 필터에 없음 → 4-6 ② 의 필터 갱신 한 줄 실행 |
| `hello_server: not found` | `chmod 755` 누락 또는 `adb push` 경로 확인 |
