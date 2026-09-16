# 실습 10. VSync 애니메이션 — DisplayEventReceiver fd 를 Looper 에 addFd

> **소요시간:** 30분 · **난이도:** ★★★ · **챕터:** Ch 3 Graphics ↔ Ch 1 Looper
> **디렉토리:** `~/android/day3/vsync_anim/vsync_anim.cpp` · 모듈 `sf_vsync_anim`

## 목표

- SurfaceFlinger 의 VSync 이벤트 fd 를 **실습 5 ④ 의 `LooperCallback`** 으로 받아 매 프레임 위치를 바꾼다.
- `sp<>`(RefBase) 와 `unique_ptr`(non-RefBase) 을 구분해 쓴다.
- 60 fps 의 16.67 ms 예산과 Jank 를 실습 13 데모로 잇는다.

---

## Step 1. 소스

**`vsync_anim/vsync_anim.cpp`**

```cpp
#define LOG_TAG "sf_vsync_anim"
#include <iostream>
#include <memory>
#include <gui/SurfaceComposerClient.h>
#include <gui/Surface.h>
#include <gui/DisplayEventReceiver.h>
#include <ui/DynamicDisplayInfo.h>
#include <utils/Looper.h>
#include <log/log.h>

using namespace android;

// ★ 실습 5 ④ 의 MyHandler = Android LooperCallback
class VSyncHandler : public LooperCallback {
public:
    VSyncHandler(SurfaceComposerClient::Transaction& tx, const sp<SurfaceControl>& sc,
                 DisplayEventReceiver* recv, int32_t screenW)
        : mSurface(sc), mReceiver(recv), mScreenW(screenW), mTx(tx) {}

    int handleEvent(int /*fd*/, int /*events*/, void* /*data*/) override {
        DisplayEventReceiver::Event ev;
        while (mReceiver->getEvents(&ev, 1) == 1) {                  // fd 에서 이벤트 읽기
            if (ev.header.type == DisplayEventReceiver::DISPLAY_EVENT_VSYNC) {
                step();
                mReceiver->requestNextVsync();                       // ★ 다음 VSync 요청 (1회성)
            }
        }
        return 1;                                                    // 1 = fd 콜백 유지
    }
private:
    void step() {
        mX = (mX + 4) % mScreenW;                                    // 60 fps × 4 px = 240 px/s
        mTx.setPosition(mSurface, mX, 200).apply(false);
    }
    sp<SurfaceControl>     mSurface;      // RefBase → sp
    DisplayEventReceiver*  mReceiver;     // non-RefBase → raw (소유자는 main 의 unique_ptr)
    int32_t mScreenW, mX = 0;
    SurfaceComposerClient::Transaction& mTx;
};

int main() {
    // 화면 폭
    auto ids = SurfaceComposerClient::getPhysicalDisplayIds();
    if (ids.empty()) { ALOGE("no display"); return -1; }
    ui::DynamicDisplayInfo ddi;
    SurfaceComposerClient::getDynamicDisplayInfoFromId(ids.front().value, &ddi);
    const int32_t screenW = ddi.getActiveDisplayMode()->resolution.getWidth();

    // Layer (실습 9)
    sp<SurfaceComposerClient> client = new SurfaceComposerClient();
    if (client->initCheck() != NO_ERROR) return -1;
    sp<SurfaceControl> sc = client->createSurface(String8("vsync_rect"), 128, 128, PIXEL_FORMAT_RGBA_8888, 0);
    if (sc == nullptr || !sc->isValid()) return -1;

    auto tx = std::make_unique<SurfaceComposerClient::Transaction>();
    tx->setLayer(sc, 0x7FFFFFFF).show(sc).apply();

    sp<ANativeWindow> surf = sc->getSurface();
    ANativeWindow_Buffer buf;
    if (ANativeWindow_lock(surf.get(), &buf, nullptr) < 0) return -1;
    uint32_t* p = static_cast<uint32_t*>(buf.bits);
    for (int y = 0; y < buf.height; ++y)
        for (int x = 0; x < buf.width; ++x) p[y * buf.stride + x] = 0xFF00FF00;   // ABGR → 초록
    ANativeWindow_unlockAndPost(surf.get());

    // ★ VSync 수신 fd
    std::unique_ptr<DisplayEventReceiver> recv = std::make_unique<DisplayEventReceiver>();
    if (recv->initCheck() != NO_ERROR) { ALOGE("DisplayEventReceiver init failed"); return -1; }
    recv->setVsyncRate(1);                 // 매 VSync (2 면 절반)
    recv->requestNextVsync();

    // ★ 실습 5 — Looper 에 fd 등록
    sp<Looper> looper = Looper::prepare(0);
    looper->addFd(recv->getFd(), 0, Looper::EVENT_INPUT,
                  new VSyncHandler(*tx, sc, recv.get(), screenW), nullptr);

    const nsecs_t until = systemTime() + seconds_to_nanoseconds(10);
    while (systemTime() < until) looper->pollAll(-1);          // ★ 실습 5 ①~③ 의 루프
    return 0;
}
```

## Step 2. 빌드 · 실행

```bash
m sf_vsync_anim
adb push $OUT/system/bin/sf_vsync_anim /data
adb shell chmod 755 /data/sf_vsync_anim
adb shell /data/sf_vsync_anim
```

초록 사각형이 10초 동안 왼쪽 → 오른쪽으로 움직이다 종료된다.

## Step 3. 관찰

```bash
adb shell dumpsys SurfaceFlinger --latency vsync_rect | head      # VSync 주기(ns) 와 프레임 타임스탬프
adb shell dumpsys display | grep -E "refreshRate|mode"            # 60 Hz → 16666667 ns
```

## Step 4. 실험

| 실험 | 관찰 |
|---|---|
| `setVsyncRate(2)` | 30 fps — 이동 속도 절반 |
| `mX += 8` | 480 px/s |
| `handleEvent` 에 `usleep(20000)` | 16.67 ms 예산 초과 → 프레임 드롭·끊김 (실습 13 Perfetto 로 확인) |
| `requestNextVsync()` 제거 | 첫 VSync 한 번만 오고 멈춤 — 1회성 요청임 |
| `recv` 를 `sp` 로 바꾸려 하면 | 컴파일 오류 — `DisplayEventReceiver` 는 `RefBase` 가 아니다 |

---

## 확인 포인트

- [ ] 사각형이 부드럽게 이동, 10초 후 종료
- [ ] `--latency` 에 16666667 (60 Hz) 주기
- [ ] `setVsyncRate(2)` 로 속도 절반
- [ ] `usleep` 으로 Jank 재현

## 핵심 정리

- `DisplayEventReceiver` = SurfaceFlinger 가 주는 **VSync fd** (Binder 로 fd 를 받아 온다 — Day 1 FD 전달).
- `Looper::addFd(fd, …, LooperCallback)` — 실습 5 ④ 그대로. `handleEvent` 가 Looper Thread(여기선 main) 에서 실행된다.
- `requestNextVsync()` 는 1회성 — 매 프레임 다시 요청. Java `Choreographer.postFrameCallback` 의 실체.
- `sp<SurfaceControl>` vs `unique_ptr<DisplayEventReceiver>` — `RefBase` 를 상속했는가로 결정.
