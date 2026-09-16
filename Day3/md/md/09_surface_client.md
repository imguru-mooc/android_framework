# 실습 9. SurfaceFlinger Client — SurfaceComposerClient · Transaction · ANativeWindow

> **소요시간:** 30분 · **난이도:** ★★★ · **챕터:** Ch 3 Graphics
> **디렉토리:** `~/android/day3/surface1/main.cpp` · 모듈 `surface_client_test`

## 목표

- Java/WMS 없이 **Native 프로세스가 SurfaceFlinger 에 직접 Layer 를 만든다**.
- `Transaction` 으로 z-order·위치·가시성을 원자적으로 적용하고, `ANativeWindow` 로 픽셀을 그린다.
- Layer 의 수명 = `sp<SurfaceControl>` 의 수명 (실습 3 연결).

## 구조

```
surface_client_test ── Binder ──▶ SurfaceFlinger
  SurfaceComposerClient           Layer "My Cpp Surface" (z=MAX, pos 100,100)
  createSurface → sp<SurfaceControl>
  Transaction.apply()             ← 속성 변경
  ANativeWindow_lock/unlockAndPost ← BufferQueue 에 버퍼 queue → 합성
```

---

## Step 1. 소스

**`surface1/main.cpp`**

```cpp
#include <iostream>
#include <unistd.h>
#include <binder/ProcessState.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/Surface.h>
#include <android/native_window.h>

using namespace android;

int main() {
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();                    // SurfaceFlinger 콜백 수신용

    // 1. SurfaceFlinger 연결 (Binder)
    sp<SurfaceComposerClient> client = new SurfaceComposerClient();
    if (client->initCheck() != NO_ERROR) { std::cerr << "SF connect failed\n"; return -1; }

    // 2. Layer 생성
    sp<SurfaceControl> sc = client->createSurface(
            String8("My Cpp Surface"), 400, 400, PIXEL_FORMAT_RGBA_8888, 0);
    if (sc == nullptr || !sc->isValid()) { std::cerr << "createSurface failed\n"; return -1; }

    // 3. 속성 — Transaction 으로 원자적 적용
    SurfaceComposerClient::Transaction tx;
    tx.setLayer(sc, 0x7FFFFFFF)          // z-order 최상위
      .setPosition(sc, 100, 100)
      .show(sc)
      .apply();

    // 4. 픽셀 쓰기 — BufferQueue 에서 버퍼 dequeue
    sp<ANativeWindow> window = sc->getSurface();     // sp<Surface> = ANativeWindow
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window.get(), &buffer, nullptr) < 0) { std::cerr << "lock failed\n"; return -1; }

    uint8_t* pixels = reinterpret_cast<uint8_t*>(buffer.bits);
    for (int y = 0; y < buffer.height; ++y)
        for (int x = 0; x < buffer.width; ++x) {
            uint8_t* px = pixels + (y * buffer.stride + x) * 4;   // ★ stride ≠ width 일 수 있다
            px[0] = 255; px[1] = 0; px[2] = 0; px[3] = 255;       // R G B A
        }

    ANativeWindow_unlockAndPost(window.get());       // ★ queue → SurfaceFlinger 가 다음 VSync 에 합성
    std::cout << "Surface drawn. Ctrl+C to exit.\n";
    while (true) sleep(1);                           // 프로세스가 살아 있는 동안 Layer 유지
    return 0;
}
```

## Step 2. 빌드 · 실행

```bash
m surface_client_test
adb push $OUT/system/bin/surface_client_test /data
adb shell chmod 755 /data/surface_client_test
adb shell /data/surface_client_test
```

Emulator 화면 (100,100) 에 빨간 400×400 사각형이 Car Launcher **위에** 나타난다. `Ctrl+C` 로 종료하면 사라진다.

## Step 3. 관찰

```bash
adb shell dumpsys SurfaceFlinger --list | grep "My Cpp"
adb shell dumpsys SurfaceFlinger | grep -A25 "My Cpp Surface" | head -40
# z=2147483647  pos=(100,100)  size=400x400  format=RGBA_8888  activeBuffer=[400x400:...]
```

## Step 4. 실험

| 실험 | 관찰 |
|---|---|
| `setLayer(sc, 0)` | Launcher 아래로 들어가 안 보임 (z-order) |
| `setAlpha(sc, 0.5f)` 추가 | 반투명 |
| `stride` 대신 `width` 로 계산 | 사각형이 비뚤어지거나 crash — 버퍼 한 줄이 width 보다 길 수 있다 |
| `while(true)` 삭제 | 프로세스 종료 → `sp<SurfaceControl>` 소멸 → Layer 즉시 제거 |
| `ANativeWindow_lock` 두 번 연속 | 두 번째는 **다른 버퍼**(triple buffering) — 이전 픽셀이 없다 |

---

## 확인 포인트

- [ ] 빨간 사각형 표시, 종료 시 사라짐
- [ ] `dumpsys SurfaceFlinger` 에서 Layer 이름·z·pos 확인
- [ ] `stride` 실험으로 버퍼 레이아웃 이해

## 핵심 정리

| API | 역할 |
|---|---|
| `SurfaceComposerClient` | SurfaceFlinger 의 `ISurfaceComposer` Binder Proxy |
| `createSurface` → `SurfaceControl` | Layer 핸들. `RefBase` — 마지막 `sp` 가 사라지면 Layer 제거 |
| `Transaction` | 여러 속성을 모아 `apply()` 시 한 VSync 에 원자 적용 |
| `getSurface()` → `Surface` = `ANativeWindow` | BufferQueue 의 producer 쪽 |
| `lock` / `unlockAndPost` | dequeueBuffer + mmap / queueBuffer — 버퍼 자체는 `dmabuf` FD 로 SurfaceFlinger 와 공유 (복사 없음) |

App 의 `SurfaceView` 도 결국 WMS 가 같은 `createSurface` 를 대신 불러 주는 것이다 (실습 8 ③ 의 `Surface.mNativeObject`).
