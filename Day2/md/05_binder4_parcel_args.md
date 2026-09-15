# 실습 5. Parcel 로 인자·반환값 전달 — ledOn(ratio) → int

> **소요시간:** 20분 · **난이도:** ★★☆ · **챕터:** Ch 1 Native Binder
> **디렉토리:** `~/aosp/day2/binder4` (실습 4 소스를 복사해 시작)

## 목표

- Proxy 에서 `Parcel::writeInt32`, Stub 에서 `readInt32` 로 인자를 전달한다.
- `reply` Parcel 로 반환값을 돌려받는다.
- Parcel 의 **쓰기 순서 = 읽기 순서** 규칙을 체험한다.

---

## Step 1. 실습 4 복사

```bash
cd ~/aosp/day2
cp binder3/*.cpp binder3/*.h binder4/
```

## Step 2. 인터페이스 변경

**`binder4/ILedService.h`** — 시그니처만 수정

```cpp
virtual int ledOn(int ratio) = 0;      // 밝기 0~100 → 실제 적용된 값 반환
```

## Step 3. Proxy / Stub 수정

**`binder4/ILedService.cpp`**

```cpp
// Proxy
int ledOn(int ratio) override {
    Parcel data, reply;
    data.writeInterfaceToken(ILedService::getInterfaceDescriptor());
    data.writeInt32(ratio);                              // ★ 인자 직렬화
    status_t st = remote()->transact(LED_ON, data, &reply);
    if (st != NO_ERROR) return -1;
    return reply.readInt32();                            // ★ 반환값 역직렬화
}

// Stub
case LED_ON: {
    CHECK_INTERFACE(ILedService, data, reply);
    int ratio = data.readInt32();                        // ★ 쓴 순서대로 읽는다
    int applied = ledOn(ratio);
    reply->writeInt32(applied);                          // ★ reply 에 쓰기
    return NO_ERROR;
}
```

## Step 4. 구현체 · Client 수정

**`binder4/LedService.h` / `LedService.cpp`**

```cpp
int ledOn(int ratio) override;

int LedService::ledOn(int ratio) {
    int applied = ratio < 0 ? 0 : (ratio > 100 ? 100 : ratio);   // 범위 보정
    printf("LedService::ledOn(%d) -> applied %d\n", ratio, applied); fflush(stdout);
    return applied;
}
```

**`binder4/my_client.cpp`**

```cpp
int main(int argc, char** argv) {
    int ratio = argc > 1 ? atoi(argv[1]) : 50;
    sp<ProcessState> proc(ProcessState::self());
    sp<IBinder> p = defaultServiceManager()->checkService(String16("my.led_service"));
    if (p == nullptr) { printf("not found\n"); return 1; }
    sp<ILedService> pLed = interface_cast<ILedService>(p);
    int applied = pLed->ledOn(ratio);
    printf("client: ledOn(%d) = %d\n", ratio, applied);
    return 0;
}
```

(`#include <cstdlib>` 추가)

## Step 5. 빌드 · 실행

```bash
cd ~/aosp/day2 && mm -j$(nproc)
```

```bash
adb shell /data/my_server_test_4 &
adb shell /data/my_client_test_4 70      # client: ledOn(70) = 70
adb shell /data/my_client_test_4 250     # client: ledOn(250) = 100
adb shell service call my.led_service 1 s16 "android.my.ILedService" i32 30
# Result: Parcel(00000000 0000001e '........')   ← 0x1e = 30
```

## Step 6. 순서를 어기면?

Stub 의 `readInt32()` 앞에 `data.readString16()` 을 한 줄 넣고 빌드해 보자. 값이 엉뚱하게 나오거나 `ratio` 가 0 이 된다. Parcel 은 **타입 정보가 없는 바이트 스트림**이라 쓰기·읽기 순서가 계약이다. AIDL 이 이 코드를 자동 생성해 주는 이유다.

---

## 확인 포인트

- [ ] `ledOn(70) = 70`, `ledOn(250) = 100` (Server 에서 보정된 값이 돌아옴)
- [ ] `service call … i32 30` 결과 Parcel 의 `0x1e`
- [ ] 읽기 순서를 바꾸면 값이 깨진다

## 핵심 정리

- `Parcel` 쓰기 API: `writeInt32 / writeInt64 / writeString16 / writeStrongBinder / writeFileDescriptor …`
- 읽기는 같은 순서의 `read*`. 순서·타입이 어긋나도 컴파일러는 모른다.
- `reply` 도 Parcel — 반환값은 `reply->write*` 로 싣고 Proxy 가 `reply.read*` 로 꺼낸다.
