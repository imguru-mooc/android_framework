# 실습 6. Car SystemUI RRO — aapt 로 리소스 이름 찾기 · /system/app 설치

> **소요시간:** 30분 · **난이도:** ★★★ · **챕터:** Ch 1·2 · 저장소 SystemUI 실습 2
> **전제:** 실습 1 의 2단계 remount 완료 (`remount succeeded`)

## 목표

- Car SystemUI 는 `<overlayable>` 을 선언하지 않아 사용자 오버레이가 `---` 가 된다는 것을 확인한다.
- `aapt dump resources` 로 **실제 리소스 이름**을 뽑아 오버레이를 만든다.
- `/system/app/` 에 설치(policy system)해 활성화하고 시계·아이콘 색이 바뀌는 것을 본다.

---

## Step 1. 현재 오버레이 구조

```bash
adb shell
cmd overlay list | grep -A12 "^com.android.systemui$"
# com.android.systemui
# [x] com.android.systemui.car.rro
# [x] com.android.systemui.auto_generated_rro_product__
# --- com.android.car.ui.overlay.sdk.carsystemui          ← 매칭 실패
# [ ] com.android.systemui.googlecarui.theme.orange.rro   ← 비활성

dumpsys overlay | grep -A6 "com.android.systemui.car.rro" | grep -E "Target|BaseCodePath"
# mTargetOverlayableName.: null                           ← ★ overlayable 미선언
# mBaseCodePath..........: /system/app/CarSystemUIRRO/CarSystemUIRRO.apk   ← system 파티션
exit
```

| 설치 위치 | policy | overlayable 필요 | 결과 |
|---|---|---|---|
| `/system/app/` | system | 불필요 | `[x]`/`[ ]` |
| `adb install` | public | **필수** | `---` |

## Step 2. 실제 리소스 이름 추출

```bash
adb shell pm path com.android.systemui
# package:/system_ext/priv-app/CarSystemUI/CarSystemUI.apk
```

```bat
adb pull /system_ext/priv-app/CarSystemUI/CarSystemUI.apk C:\aosp16\bin\
aapt dump resources C:\aosp16\bin\CarSystemUI.apk | findstr /i "color/system_bar color/car_nav color/car_status"
```

```text
  resource 0x7f06… com.android.systemui:color/system_bar_clock_text_color
  resource 0x7f06… com.android.systemui:color/system_bar_icon_color
  resource 0x7f06… com.android.systemui:color/system_bar_text_color
  resource 0x7f06… com.android.systemui:color/car_nav_icon_fill_color
  resource 0x7f06… com.android.systemui:color/car_nav_icon_fill_color_selected
  resource 0x7f06… com.android.systemui:color/car_status_icon_color
```

Phone SystemUI 와 이름이 다르다. **여기서 나온 이름만** 오버레이에 쓴다.

## Step 3. SystemUIOverlay 프로젝트 (Android Studio)

| 항목 | 값 |
|---|---|
| Name / Package | `SystemUIOverlay` / `com.example.systemuioverlay` · Java · minSdk 34 |

`MainActivity`·layout 삭제.

**`AndroidManifest.xml`** — `targetName` **없음**

```xml
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android">
    <application android:hasCode="false" android:label="SystemUI Color Overlay"/>
    <overlay
        android:targetPackage="com.android.systemui"
        android:priority="1"
        android:isStatic="false"/>
</manifest>
```

**`res/values/colors.xml`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <!-- 상단바 -->
    <color name="system_bar_clock_text_color">#FFFF4081</color>   <!-- 시계 → 핑크 -->
    <color name="status_bar_clock_color">#FFFF4081</color>
    <color name="system_bar_icon_color">#FFFFEB3B</color>         <!-- BT/Wi-Fi 아이콘 → 노랑 -->
    <color name="car_status_icon_color">#FFFFEB3B</color>
    <color name="system_bar_text_color">#FF00E5FF</color>         <!-- "Driver" → 시안 -->
    <!-- 하단 내비바 -->
    <color name="car_nav_icon_fill_color">#FFFFAB40</color>
    <color name="car_nav_icon_fill_color_selected">#FFFFFF00</color>
    <color name="car_nav_icon_background_color">#FF1A237E</color>
    <color name="car_nav_icon_background_color_selected">#FFFF6D00</color>
    <!-- 바 배경 (투명 구조라 함께 넣어야 효과가 나기도 한다) -->
    <color name="system_bar_background_opaque">#FF1A237E</color>
    <color name="system_bar_background_transparent">#FF1A237E</color>
    <color name="control_bar_background_color">#FF0D47A1</color>
</resources>
```

Generate APKs → `C:\aosp16\bin\systemuioverlay.apk`.

## Step 4. 시스템 앱으로 설치

```bat
adb root
adb remount
adb shell mkdir -p /system/app/SystemUIOverlay
adb push C:\aosp16\bin\systemuioverlay.apk /system/app/SystemUIOverlay/SystemUIOverlay.apk
adb reboot
adb wait-for-device
```

## Step 5. 활성화

```bash
adb shell
cmd overlay list | grep systemuioverlay
# [ ] com.example.systemuioverlay          ← 매칭 성공 (--- 가 아니다)
cmd overlay enable com.example.systemuioverlay
killall com.android.systemui                 # SystemUI 재시작 → 반영
```

| 영역 | 변경 후 |
|---|---|
| 상단 시계 | 핑크 |
| BT/Wi-Fi 아이콘 · 하단 숫자 | 노랑 |
| "Driver" | 시안 |
| 하단 내비 아이콘 | 주황 |
| 바 배경 | 대부분 변경 없음 — 투명 구조 |

```bash
screencap -p /sdcard/after.png
cmd overlay dump com.example.systemuioverlay | grep -E "State|Priority"
exit
```

```bat
adb pull /sdcard/after.png C:\aosp16\
```

## Step 6. 수정 · 제거

색을 바꿔 다시 배포: `adb root; adb remount` → `adb push … /system/app/SystemUIOverlay/` → `adb reboot` → `enable` → `killall`.

제거: `cmd overlay disable com.example.systemuioverlay` → `adb shell rm -rf /system/app/SystemUIOverlay` → `adb reboot`.

---

## 확인 포인트

- [ ] `mTargetOverlayableName: null` 확인
- [ ] `aapt` 출력에서 `system_bar_clock_text_color` 등 실제 이름
- [ ] `/system/app` 설치 후 `[ ]` → `enable` → 시계·아이콘 색 변경

## 트러블슈팅

| 증상 | 원인 | 해결 |
|---|---|---|
| `---` | `targetName` 을 넣었거나 사용자 설치 | `targetName` 제거 + `/system/app` |
| `SecurityException` on enable | `adb install` 로 설치됨 | `/system/app` 로 |
| `Read-only file system` / `mkdir failed` | 2차 remount 누락 | `adb root; adb remount` |
| 색 변화 없음 | 이름 불일치 | `aapt dump resources` 재확인 · `killall` |
| 바 배경 그대로 | 투명 리소스 사용 | `*_transparent` 도 함께 |
