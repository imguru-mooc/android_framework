# [D4-5] 실습 N-2 — surface_test: C++로 화면에 그리기

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★★☆ | Day 4 오후 1교시 · ★전원 필수 | 배포 `surface_test/` + 서버/WinSCP/로컬 · 🎬 ⑨ 선시청 |

> 🎯 **실습 목표** — Activity도 Java도 없이 **SurfaceComposerClient**로 SurfaceFlinger에 Layer를 만들어 붉은 400×400 사각형을 화면 최상단에 띄운다.

## Step 1. main.cpp 핵심 읽기 (vendor/edu/surface_test/surface1/)

```cpp
sp<SurfaceComposerClient> client = new SurfaceComposerClient();
sp<SurfaceControl> ctrl = client->createSurface(
    String8("My Cpp Surface"), 400, 400, PIXEL_FORMAT_RGBA_8888);

SurfaceComposerClient::Transaction{}
    .setLayer(ctrl, INT_MAX)          // ★ 최상단 z-order
    .setPosition(ctrl, 100, 100)
    .show(ctrl)
    .apply();

ANativeWindow_Buffer buf;
sp<Surface> surface = ctrl->getSurface();
surface->lock(&buf, nullptr);
// 픽셀 전체를 0xFF0000FF(불투명 빨강)로 채우기
uint32_t* px = (uint32_t*)buf.bits;
for (int i = 0; i < buf.stride * buf.height; ++i) px[i] = 0xFF0000FF;
surface->unlockAndPost();             // ★ BufferQueue 제출 → SF가 합성
sleep(30);
```

## Step 2. Android.bp + 빌드

```text
cc_binary {
    name: "surface1",
    srcs: ["main.cpp"],
    shared_libs: ["libgui", "libui", "libbinder", "libutils", "liblog"],
}
```

```bash
mm    # (서버, 해당 디렉토리)
```

## Step 3. 전송 → 실행 → 화면 확인

🖱 WinSCP **OUT** → `system/bin/surface1` 다운로드

```bat
adb root
adb push surface1 /data/local/tmp/ & adb shell chmod +x /data/local/tmp/surface1
adb shell /data/local/tmp/surface1
```

✅ **예상 결과:** 로컬 Automotive Emulator 화면 (100,100) 위치에 **붉은 400×400 사각형**이 30초간 최상단 표시 — 앱 없이 그렸다!

## Step 4. SF 장부에서 확인

```bat
adb shell dumpsys SurfaceFlinger --list | findstr /i "Cpp"
```

✅ **예상 결과:** `My Cpp Surface#0` — ⑨ 애니메이션의 그 난입 Layer가 실제 장부에

# 🏁 Pass 판정 체크리스트

- [ ] 붉은 사각형 화면 표시 (스크린샷)
- [ ] dumpsys에 "My Cpp Surface" 라인
- [ ] setLayer 값을 0으로 바꿔 재실행 → 다른 창 뒤로 숨는 것 확인(z-order 체득)

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| 링크 에러(libgui) | shared_libs 누락 | Step 2 목록 확인 |
| 실행되나 화면 없음 | show/apply 누락, z 낮음 | Transaction 체인·INT_MAX 확인 |
| 색이 파랑으로 보임 | RGBA 바이트 순서 | 0xFF0000FF ↔ 0xFFFF0000 실험으로 채널 확인 |
| SELinux denied | 개발 검증 | setenforce 0 실험 후 복구(N-1과 동일 토론) |

# 🚗 현업 활용 포인트

💡 **부팅 애니메이션·클러스터 경고 아이콘**이 정확히 이 계층에서 그려집니다(Java Framework 기동 전/밖). "화면 문제 = 앱 문제"라는 등식을 오늘 영구 폐기하세요.

---
*실습 D4-5 (34/36) · 다음: **D4-6 Lab 4-3 APEX 빌드***
