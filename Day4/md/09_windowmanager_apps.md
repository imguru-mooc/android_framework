# 실습 9. WindowManager 앱 — 플로팅 오버레이 창 · 분할 화면

> **소요시간:** 25분 · **난이도:** ★★☆ · **챕터:** Ch 2 · 저장소 `FloatingOverlayDemo.zip` · `SplitScreenDemo.zip`

## 목표

- 다른 앱 위에 떠 있는 창(`TYPE_APPLICATION_OVERLAY`)을 만들고, 권한(`SYSTEM_ALERT_WINDOW`)이 WMS 에서 검사되는 것을 본다.
- WMS 가 만든 창이 Day 3 의 SurfaceFlinger Layer 로 보이는 것을 `dumpsys` 로 잇는다.
- 앱 내 분할 화면은 Layer 가 하나임을 확인한다.

---

## Step 1. FloatingOverlayDemo 열기 (Android Studio)

`C:\aosp16\day4\FloatingOverlayDemo` 를 열고 핵심 코드를 확인한다.

**`FloatingService.java`**

```java
public class FloatingService extends Service {
    private WindowManager windowManager;
    private View floatingView;

    @Override public IBinder onBind(Intent intent) { return null; }

    @Override public void onCreate() {
        super.onCreate();
        floatingView = LayoutInflater.from(this).inflate(R.layout.floating_layout, null);
        WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.WRAP_CONTENT, WindowManager.LayoutParams.WRAP_CONTENT,
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,   // ★ 앱 위에 뜨는 창 타입 (2038)
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                PixelFormat.TRANSLUCENT);
        params.gravity = Gravity.TOP | Gravity.LEFT; params.x = 100; params.y = 100;
        windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
        windowManager.addView(floatingView, params);                  // ★ WMS 에 창 요청 → SF Layer
    }

    @Override public void onDestroy() { super.onDestroy(); if (floatingView != null) windowManager.removeView(floatingView); }
}
```

**`MainActivity.java`** — `Settings.canDrawOverlays()` 가 false 면 설정 화면으로, true 면 `startService(FloatingService)`.

Manifest: `<uses-permission android:name="android.permission.SYSTEM_ALERT_WINDOW"/>`.

Run → Emulator 에 설치.

## Step 2. 권한 — appops 로 허용

Automotive 설정 화면에 오버레이 허용 UI 가 없을 수 있으므로 appops 로 준다.

```bash
adb shell
appops set com.example.floatingoverlaydemo SYSTEM_ALERT_WINDOW allow
am force-stop com.example.floatingoverlaydemo
am start -n com.example.floatingoverlaydemo/.MainActivity
```

홈으로 나가도 왼쪽 위에 작은 창이 남아 있다.

## Step 3. WMS 창 → SF Layer

```bash
dumpsys window windows | grep -B2 -A8 "floatingoverlay" | head -30
#  Window #… Window{… u0 com.example.floatingoverlaydemo}:
#    ty=APPLICATION_OVERLAY (2038) fl=… FLAG_NOT_FOCUSABLE
#    mHasSurface=true …

dumpsys SurfaceFlinger --list | grep -i floating
# com.example.floatingoverlaydemo#0                 ← ★ Day 3 실습 9 의 createSurface 를 WMS 가 대신 한 것

dumpsys SurfaceFlinger | grep -A12 'floatingoverlaydemo' | grep -E "z=|pos=|size="
```

Activity 창과 오버레이 창의 `z` 값을 비교해 오버레이가 위에 있는 이유를 본다.

## Step 4. 권한 거부 재현

```bash
am force-stop com.example.floatingoverlaydemo
appops set com.example.floatingoverlaydemo SYSTEM_ALERT_WINDOW deny
am start -n com.example.floatingoverlaydemo/.MainActivity
logcat -d -s AndroidRuntime | grep -A5 BadTokenException | head
# android.view.WindowManager$BadTokenException: Unable to add window … permission denied for window type 2038
appops set com.example.floatingoverlaydemo SYSTEM_ALERT_WINDOW allow
```

권한 검사는 App 이 아니라 **WMS**(system_server) 에서 `addView` 의 Binder 호출을 받을 때 한다 — Day 2 실습 8 의 UID 검사와 같은 위치.

## Step 5. SplitScreenDemo (5분)

`C:\aosp16\day4\SplitScreenDemo` 를 열어 Run. `LinearLayout` 안 좌우 `FrameLayout` 에 `MapFragment`/`MediaFragment`.

```bash
dumpsys SurfaceFlinger --list | grep -i splitscreen
# com.example.splitscreendemo/…MainActivity#0       ← ★ Layer 는 하나 — 분할은 View 계층의 일
dumpsys display | grep -E "mDisplayId|DisplayDeviceInfo" | head    # Automotive 디스플레이 목록
exit
```

Automotive 멀티 디스플레이(클러스터·뒷좌석)는 디스플레이마다 별도 Layer 스택이며 `am start --display <id>` 로 띄운다 — 오늘은 목록만 확인한다.

---

## 확인 포인트

- [ ] 플로팅 창이 다른 앱 위에 유지, `dumpsys window` 에 `APPLICATION_OVERLAY (2038)`
- [ ] `dumpsys SurfaceFlinger --list` 에 오버레이 Layer
- [ ] `deny` 시 `BadTokenException`
- [ ] SplitScreen 은 Layer 하나

## 핵심 정리

| 항목 | 내용 |
|---|---|
| `WindowManager.addView` | App → WMS(Binder) → WMS 가 SF 에 `createSurface` → Layer. App 은 `Surface` 를 받아 그린다 |
| 창 타입 | `TYPE_APPLICATION`(Activity) · `TYPE_APPLICATION_OVERLAY`(권한 필요) · 시스템 창(StatusBar 등, system UID) |
| `SYSTEM_ALERT_WINDOW` | 설치 시 자동 부여 안 됨 — 사용자 허용 또는 `appops` |
| 분할 vs 멀티 디스플레이 | View 분할은 Layer 1개 · 디스플레이별 Layer 스택은 별개 |
