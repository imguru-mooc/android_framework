# 실습 6. 역방향 호출 — Callback 인터페이스를 writeStrongBinder 로 전달

> **소요시간:** 30분 · **난이도:** ★★★ · **챕터:** Ch 1 Native Binder
> **디렉토리:** `~/aosp/day2/binder5`

## 목표

- Client 가 자기 Binder 객체(`BnMyDataCallback`)를 Server 에 넘기고, Server 가 그것을 통해 **Client 를 호출**한다.
- `writeStrongBinder` / `readStrongBinder` 와 `interface_cast` 로 IBinder 가 Parcel 을 타는 것을 본다.
- Client 도 `joinThreadPool()` 이 필요한 이유를 이해한다.

## 구조

```
Client                                        Server
 MyCallback : BnMyDataCallback                 MyDataService : BnMyDataService
 registerCallback(cb) ─── writeStrongBinder ──▶ readStrongBinder → interface_cast<IMyDataCallback>
 onDataReceived("Hello…") ◀── BpMyDataCallback::onDataReceived ── cb->onDataReceived()
 (Client 의 Binder Thread 에서 실행)
```

---

## Step 1. Callback 인터페이스

**`binder5/IMyDataCallback.h`**

```cpp
#ifndef IMY_DATA_CALLBACK_H
#define IMY_DATA_CALLBACK_H
#include <binder/IInterface.h>
#include <utils/String16.h>

namespace android {
class IMyDataCallback : public IInterface {
public:
    DECLARE_META_INTERFACE(MyDataCallback);
    virtual void onDataReceived(const String16& data) = 0;
    enum { ON_DATA_RECEIVED = IBinder::FIRST_CALL_TRANSACTION };
};
class BnMyDataCallback : public BnInterface<IMyDataCallback> {
public:
    status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags = 0) override;
};
}
#endif
```

**`binder5/IMyDataCallback.cpp`**

```cpp
#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES
#include <binder/Parcel.h>
#include "IMyDataCallback.h"

namespace android {

class BpMyDataCallback : public BpInterface<IMyDataCallback> {
public:
    explicit BpMyDataCallback(const sp<IBinder>& impl) : BpInterface<IMyDataCallback>(impl) {}
    void onDataReceived(const String16& data) override {
        Parcel p, reply;
        p.writeInterfaceToken(IMyDataCallback::getInterfaceDescriptor());
        p.writeString16(data);
        remote()->transact(ON_DATA_RECEIVED, p, &reply);     // ★ Server → Client 방향 IPC
    }
};

status_t BnMyDataCallback::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
    switch (code) {
    case ON_DATA_RECEIVED: {
        CHECK_INTERFACE(IMyDataCallback, data, reply);
        onDataReceived(data.readString16());
        return NO_ERROR;
    }
    default: return BBinder::onTransact(code, data, reply, flags);
    }
}

IMPLEMENT_META_INTERFACE(MyDataCallback, "com.example.IMyDataCallback");
}
```

## Step 2. Service 인터페이스

**`binder5/IMyDataService.h`**

```cpp
#ifndef IMY_DATA_SERVICE_H
#define IMY_DATA_SERVICE_H
#include <binder/IInterface.h>
#include "IMyDataCallback.h"

namespace android {
class IMyDataService : public IInterface {
public:
    DECLARE_META_INTERFACE(MyDataService);
    virtual void registerCallback(const sp<IMyDataCallback>& cb) = 0;
    enum { REGISTER_CALLBACK = IBinder::FIRST_CALL_TRANSACTION };
};
class BnMyDataService : public BnInterface<IMyDataService> {
public:
    status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags = 0) override;
};
}
#endif
```

**`binder5/IMyDataService.cpp`**

```cpp
#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES
#include <binder/Parcel.h>
#include "IMyDataService.h"

namespace android {

class BpMyDataService : public BpInterface<IMyDataService> {
public:
    explicit BpMyDataService(const sp<IBinder>& impl) : BpInterface<IMyDataService>(impl) {}
    void registerCallback(const sp<IMyDataCallback>& cb) override {
        Parcel data, reply;
        data.writeInterfaceToken(IMyDataService::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(cb));     // ★ Binder 객체를 Parcel 에 싣는다
        remote()->transact(REGISTER_CALLBACK, data, &reply);
    }
};

IMPLEMENT_META_INTERFACE(MyDataService, "android.my.IMyDataService");

status_t BnMyDataService::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
    switch (code) {
    case REGISTER_CALLBACK: {
        CHECK_INTERFACE(IMyDataService, data, reply);
        sp<IBinder> binder = data.readStrongBinder();            // ★ Driver 가 handle 로 변환해 준 것
        sp<IMyDataCallback> cb = interface_cast<IMyDataCallback>(binder);   // → BpMyDataCallback
        registerCallback(cb);
        return NO_ERROR;
    }
    default: return BBinder::onTransact(code, data, reply, flags);
    }
}
}
```

## Step 3. Server / Client

**`binder5/server.cpp`**

```cpp
#include <cstdio>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include "IMyDataService.h"

using namespace android;

class MyDataService : public BnMyDataService {
public:
    void registerCallback(const sp<IMyDataCallback>& cb) override {
        printf("server: registerCallback() — calling back client\n"); fflush(stdout);
        cb->onDataReceived(String16("Hello from C++ Binder Server"));   // ★ 역방향 호출
        printf("server: callback returned\n"); fflush(stdout);
    }
};

int main() {
    defaultServiceManager()->addService(String16("my.data.service"), new MyDataService());
    printf("my.data.service registered\n"); fflush(stdout);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
```

**`binder5/client.cpp`**

```cpp
#include <cstdio>
#include <unistd.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/String8.h>
#include "IMyDataService.h"

using namespace android;

class MyCallback : public BnMyDataCallback {
public:
    void onDataReceived(const String16& data) override {
        printf("client: onDataReceived(\"%s\")  tid=%d\n", String8(data).c_str(), gettid());
        fflush(stdout);
    }
};

int main() {
    sp<ProcessState> proc(ProcessState::self());
    sp<IBinder> b = defaultServiceManager()->checkService(String16("my.data.service"));
    if (b == nullptr) { printf("not found\n"); return 1; }
    sp<IMyDataService> svc = interface_cast<IMyDataService>(b);

    sp<MyCallback> cb = new MyCallback();
    printf("client: main tid=%d, registering callback\n", gettid()); fflush(stdout);
    svc->registerCallback(cb);

    // ★ Client 도 Server 역할을 하므로 Binder Thread 가 필요하다
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    return 0;
}
```

## Step 4. 빌드 · 실행

```bash
cd ~/aosp/day2 && mm -j$(nproc)
```

```bash
adb shell /data/binder_server_5 &
adb shell /data/binder_client_5
```

```text
client: main tid=6011, registering callback
server: registerCallback() — calling back client
client: onDataReceived("Hello from C++ Binder Server")  tid=6013     ← ★ main 이 아닌 Binder Thread
server: callback returned
```

## Step 5. 실험 — Client 의 joinThreadPool 을 빼면?

`client.cpp` 마지막 두 줄을 주석 처리하고 실행하면 `registerCallback()` 이 반환되지 않거나 콜백이 오지 않는다. Client 에 요청을 받을 Binder Thread 가 없기 때문이다. Day 1 실습 9 의 Java Client 는 Framework 가 Thread Pool 을 자동으로 켜 주었지만, Native 는 직접 켜야 한다.

---

## 확인 포인트

- [ ] `onDataReceived` 가 Client 의 **다른 tid** 에서 실행
- [ ] `writeStrongBinder` 로 넘긴 객체가 Server 에서 `BpMyDataCallback` 으로 나타남
- [ ] Client `joinThreadPool` 제거 시 콜백 실패

## 핵심 정리

- IBinder 는 Parcel 에 실을 수 있는 특별한 타입이다. Driver 가 송신 측 객체 포인터를 수신 측 **handle** 로 변환한다.
- 콜백을 받는 쪽은 Server 이기도 하다 → `startThreadPool()` + `joinThreadPool()` 필수.
- Day 1 의 `RemoteCallbackList` 는 이 구조 위에 DeathRecipient 를 얹은 것이다.
