# 실습 4. Bn / Bp 대칭 완성 — BnLedService::onTransact · LedService 구현

> **소요시간:** 30분 · **난이도:** ★★☆ · **챕터:** Ch 1 Native Binder
> **디렉토리:** `~/aosp/day2/binder3`

## 목표

- Server 측 Stub `BnLedService` 를 만들어 code → Method 분기를 인터페이스 계층으로 옮긴다.
- 실제 구현체 `LedService : BnLedService` 를 분리한다 (Java 의 `Stub` 상속과 동일).
- `service list` 에 descriptor 가 나타나는 것을 확인한다.

## 구조

```
ILedService.h      ILedService (인터페이스) · BnLedService (Stub 선언)
ILedService.cpp    BpLedService (Proxy) · BnLedService::onTransact (code → 가상 함수)
LedService.h/.cpp  LedService : BnLedService  — ledOn() 실제 구현
my_server.cpp      addService("my.led_service", new LedService)
my_client.cpp      interface_cast → ledOn()
```

---

## Step 1. 인터페이스 헤더 — Bn 선언 추가

**`binder3/ILedService.h`**

```cpp
#ifndef ANDROID_ILED_SERVICE_H
#define ANDROID_ILED_SERVICE_H

#include <binder/IInterface.h>
#include <utils/String16.h>

namespace android {

class ILedService : public IInterface {
public:
    DECLARE_META_INTERFACE(LedService);
    virtual void ledOn(void) = 0;
    enum { LED_ON = IBinder::FIRST_CALL_TRANSACTION };
};

// ★ Server 측 Stub — BBinder 이면서 ILedService 이기도 하다
class BnLedService : public BnInterface<ILedService> {
public:
    status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags = 0) override;
};

}  // namespace android
#endif
```

## Step 2. Proxy + Stub 구현

**`binder3/ILedService.cpp`**

```cpp
#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES

#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <utils/Log.h>
#include "ILedService.h"

namespace android {

// ---------- Proxy (Client) ----------
class BpLedService : public BpInterface<ILedService> {
public:
    explicit BpLedService(const sp<IBinder>& impl) : BpInterface<ILedService>(impl) {}
    void ledOn(void) override {
        Parcel data, reply;
        data.writeInterfaceToken(ILedService::getInterfaceDescriptor());   // ★ 인터페이스 검증 토큰
        remote()->transact(LED_ON, data, &reply);
    }
};

IMPLEMENT_META_INTERFACE(LedService, "android.my.ILedService");

// ---------- Stub (Server) ----------
status_t BnLedService::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
    switch (code) {
    case LED_ON: {
        CHECK_INTERFACE(ILedService, data, reply);   // ★ 토큰 불일치면 PERMISSION_DENIED
        ledOn();                                     // ★ 순수 가상 → LedService::ledOn()
        return NO_ERROR;
    }
    default:
        return BBinder::onTransact(code, data, reply, flags);
    }
}

}  // namespace android
```

## Step 3. 구현체

**`binder3/LedService.h`**

```cpp
#ifndef ANDROID_LED_SERVICE_H
#define ANDROID_LED_SERVICE_H
#include "ILedService.h"

namespace android {
class LedService : public BnLedService {
public:
    void ledOn(void) override;
};
}
#endif
```

**`binder3/LedService.cpp`**

```cpp
#include <cstdio>
#include <unistd.h>
#include <binder/IPCThreadState.h>
#include "LedService.h"

namespace android {
void LedService::ledOn(void) {
    IPCThreadState* ipc = IPCThreadState::self();
    printf("LedService::ledOn()  caller pid=%d uid=%d  (tid=%d)\n",
           ipc->getCallingPid(), ipc->getCallingUid(), gettid());
    fflush(stdout);
}
}
```

## Step 4. Server / Client

**`binder3/my_server.cpp`**

```cpp
#include <cstdio>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include "LedService.h"

using namespace android;

int main() {
    sp<ProcessState> proc(ProcessState::self());
    defaultServiceManager()->addService(String16("my.led_service"), new LedService);
    printf("my.led_service registered\n"); fflush(stdout);
    ProcessState::self()->startThreadPool();      // ★ 추가 Binder Thread 생성
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
```

**`binder3/my_client.cpp`**

```cpp
#include <cstdio>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include "ILedService.h"

using namespace android;

int main() {
    sp<ProcessState> proc(ProcessState::self());
    sp<IBinder> p = defaultServiceManager()->checkService(String16("my.led_service"));
    if (p == nullptr) { printf("my.led_service not found\n"); return 1; }
    sp<ILedService> pLed = interface_cast<ILedService>(p);
    pLed->ledOn();
    printf("client: ledOn() returned\n");
    return 0;
}
```

## Step 5. 빌드 · 실행

```bash
cd ~/aosp/day2 && mm -j$(nproc)
```

```bat
adb push my_server_test_3 /data & adb push my_client_test_3 /data
adb shell chmod 755 /data/my_server_test_3 /data/my_client_test_3
```

```bash
# ①
adb shell /data/my_server_test_3
# ②
adb shell service list | grep my.led
# 131  my.led_service: [android.my.ILedService]      ← ★ descriptor 가 보인다
adb shell /data/my_client_test_3
```

## Step 6. CHECK_INTERFACE 체험

`service call` 은 인터페이스 토큰을 넣지 않는다:

```bash
adb shell service call my.led_service 1
# Result: Parcel(ffffffffffffffff ...)  → PERMISSION_DENIED (토큰 없음)
```

토큰을 직접 넣으면 통과한다:

```bash
adb shell service call my.led_service 1 s16 "android.my.ILedService"
```

---

## 확인 포인트

- [ ] `service list` 에 `[android.my.ILedService]` descriptor 표시
- [ ] 서버 로그에 `caller pid / uid` 출력, Client 가 정상 반환
- [ ] `service call … 1` (토큰 없음) 실패, `s16 "android.my.ILedService"` 붙이면 성공

## 핵심 정리

| Java (Day 1) | C++ 수작업 (오늘) |
|---|---|
| `ICalculatorService.Stub` | `BnLedService` (BnInterface) |
| `Stub.Proxy` | `BpLedService` (BpInterface) |
| `Stub.asInterface()` | `interface_cast<>` / `IMPLEMENT_META_INTERFACE` |
| `onTransact(code, data, reply)` | `BnLedService::onTransact` |
| `data.enforceInterface(DESCRIPTOR)` | `CHECK_INTERFACE` / `writeInterfaceToken` |

`BnInterface<I>` 는 `BBinder` 와 `I` 를 동시에 상속한다. 그래서 `addService` 에 `new LedService` 를 넘길 수 있고(BBinder), 같은 객체로 `ledOn()` 도 부를 수 있다(ILedService).
