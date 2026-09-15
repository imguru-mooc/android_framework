# 실습 3. Client 측 Proxy 수작업 — ILedService · BpLedService · interface_cast

> **소요시간:** 30분 · **난이도:** ★★☆ · **챕터:** Ch 1 Native Binder
> **디렉토리:** `~/aosp/day2/binder2`

## 목표

- 인터페이스 클래스 `ILedService` 와 Client 측 Proxy `BpLedService` 를 **손으로** 만든다.
- `DECLARE_META_INTERFACE` / `IMPLEMENT_META_INTERFACE` / `interface_cast<>` 가 무엇을 만들어 주는지 본다.
- Server 는 아직 raw `BBinder` (`onTransact` 직접 구현) 로 두어, Proxy 가 보낸 code 가 그대로 도착하는 것을 확인한다.

## 구조

```
Client                                   Server
 ILedService (순수 가상 인터페이스)         AAA : BBinder
   └ BpLedService : BpInterface<ILedService>   └ onTransact(code=1) → ledOn()
        ledOn() → remote()->transact(LED_ON)
 interface_cast<ILedService>(IBinder)  → BpLedService 생성
```

---

## Step 1. 인터페이스 헤더

**`binder2/ILedService.h`**

```cpp
#ifndef ANDROID_ILED_SERVICE_H
#define ANDROID_ILED_SERVICE_H

#include <binder/IInterface.h>
#include <utils/String16.h>

namespace android {

class ILedService : public IInterface {
public:
    DECLARE_META_INTERFACE(LedService);      // ★ asInterface(), getInterfaceDescriptor() 선언
    virtual void ledOn(void) = 0;
    enum {
        LED_ON = IBinder::FIRST_CALL_TRANSACTION,   // = 1
    };
};

}  // namespace android
#endif
```

## Step 2. Proxy 구현

**`binder2/ILedService.cpp`**

```cpp
#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES   // 수작업 인터페이스 허용 (Android 12+)

#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <utils/Log.h>
#include "ILedService.h"

namespace android {

// ★ Client 측 Proxy — Method 호출을 transact() 로 바꾼다
class BpLedService : public BpInterface<ILedService> {
public:
    explicit BpLedService(const sp<IBinder>& impl) : BpInterface<ILedService>(impl) {}

    void ledOn(void) override {
        Parcel data, reply;
        remote()->transact(LED_ON, data, &reply);   // code 1, 인자 없음
    }
};

// ★ asInterface(): IBinder → 같은 Process 면 로컬 객체, 아니면 new BpLedService(binder)
IMPLEMENT_META_INTERFACE(LedService, "android.my.ILedService");

}  // namespace android
```

## Step 3. Server (raw BBinder)

**`binder2/my_server.cpp`**

```cpp
#include <cstdio>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>

using namespace android;

class AAA : public BBinder {
public:
    void ledOn() { printf("AAA::ledOn()\n"); fflush(stdout); }

    status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) override {
        (void)data; (void)reply; (void)flags;
        printf("AAA::onTransact(code=%u)\n", code); fflush(stdout);
        switch (code) {
        case 1: ledOn(); return NO_ERROR;
        default: return UNKNOWN_TRANSACTION;
        }
    }
};

int main() {
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();
    sm->addService(String16("led.service"), new AAA);
    printf("led.service registered\n"); fflush(stdout);
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
```

## Step 4. Client

**`binder2/my_client.cpp`**

```cpp
#include <cstdio>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include "ILedService.h"

using namespace android;

int main() {
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();

    sp<IBinder> p = sm->checkService(String16("led.service"));
    if (p == nullptr) { printf("led.service not found\n"); return 1; }

    sp<ILedService> pLed = interface_cast<ILedService>(p);   // ★ BpLedService 생성
    printf("client: calling ledOn()\n"); fflush(stdout);
    pLed->ledOn();                                           // ★ 그냥 Method 호출처럼 보인다
    printf("client: done\n");
    return 0;
}
```

## Step 5. 빌드 · 실행

```bash
cd ~/aosp/day2 && mm -j$(nproc)
# → $OUT/system/bin/my_server_test_2, my_client_test_2
```

```bat
adb push my_server_test_2 /data
adb push my_client_test_2 /data
adb shell chmod 755 /data/my_server_test_2 /data/my_client_test_2
```

터미널 ① `adb shell /data/my_server_test_2` · 터미널 ② `adb shell /data/my_client_test_2`

```text
[①] led.service registered
[①] AAA::onTransact(code=1)
[①] AAA::ledOn()
[②] client: calling ledOn()
[②] client: done
```

---

## 확인 포인트

- [ ] Client 코드에서 `pLed->ledOn()` 한 줄이 Server 의 `onTransact(code=1)` 로 도착
- [ ] `IMPLEMENT_META_INTERFACE` 의 descriptor 문자열 `"android.my.ILedService"` — `service list` 에는 아직 안 보인다 (Server 가 raw BBinder 라 descriptor 를 모른다)
- [ ] `interface_cast` 가 반환한 객체는 `BpLedService` (다른 Process 이므로)

## 핵심 정리

| 매크로 / 함수 | 만들어 주는 것 |
|---|---|
| `DECLARE_META_INTERFACE(X)` | `static sp<IX> asInterface(sp<IBinder>)`, `getInterfaceDescriptor()` 선언 |
| `IMPLEMENT_META_INTERFACE(X, "desc")` | 위 두 함수의 구현. `asInterface` 는 `queryLocalInterface` 로 로컬/원격 분기 → 원격이면 `new BpX(binder)` |
| `interface_cast<IX>(binder)` | `IX::asInterface(binder)` 호출 |
| `BpInterface<IX>` | `remote()` 로 `BpBinder` 접근 → `transact()` |

Day 1 의 Java `Stub.asInterface()` 와 완전히 같은 구조다. 다음 실습에서 Server 쪽(`BnLedService`)을 채워 대칭을 완성한다.

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `error: manual binder interfaces are deprecated` | `ILedService.cpp` 맨 위의 `#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES` 확인 (include 보다 먼저) |
| Client 가 `not found` | 서버 터미널이 살아 있는지, 이름 `led.service` 일치 확인 |
| 링크 오류 `undefined reference to ILedService::descriptor` | `my_client_test_2` 의 `srcs` 에 `ILedService.cpp` 가 있는지 확인 |
