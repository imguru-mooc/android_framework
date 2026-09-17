# 실습 3. RRO — 문자열 · 색상 · drawable 을 런타임에 바꾸기

> **소요시간:** 35분 · **난이도:** ★★☆ · **챕터:** Ch 1
> **환경:** Android Studio (Windows) 2 프로젝트 · 저장소 `RRO_example_android16.md`

## 목표

- 타겟 앱이 `<overlayable>` 로 허용한 리소스만 RRO 가 덮어쓸 수 있다는 **계약**을 확인한다.
- `cmd overlay list / enable / disable / dump` 로 상태를 다룬다.
- 문자열·색상 → drawable(shape · 아이콘) 순으로 확장한다.

---

## Step 1. 타겟 앱 RROTarget

| 항목 | 값 |
|---|---|
| Name / Package | `RROTarget` / `com.example.rrotarget` |
| Language · minSdk | Java · **34** (36 이면 API 35 Emulator 에 `INSTALL_FAILED_OLDER_SDK`) |

**`res/layout/activity_main.xml`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<LinearLayout xmlns:android="http://schemas.android.com/apk/res/android"
    android:id="@+id/root" android:layout_width="match_parent" android:layout_height="match_parent"
    android:background="@drawable/background_shape" android:orientation="vertical"
    android:gravity="center" android:padding="32dp">
    <ImageView android:id="@+id/icon" android:layout_width="96dp" android:layout_height="96dp"
        android:src="@drawable/ic_target_icon" android:contentDescription="icon"/>
    <TextView android:id="@+id/hello_text" android:layout_width="wrap_content" android:layout_height="wrap_content"
        android:text="@string/hello_world" android:textColor="@color/text_color" android:textSize="28sp"
        android:layout_marginTop="24dp"/>
</LinearLayout>
```

**`res/values/strings.xml`**

```xml
<resources>
    <string name="app_name">RROTarget</string>
    <string name="hello_world">how are you</string>
</resources>
```

**`res/values/colors.xml`**

```xml
<resources>
    <color name="background_color">#FFFFFFFF</color>
    <color name="text_color">#FF000000</color>
</resources>
```

**`res/drawable/background_shape.xml`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<shape xmlns:android="http://schemas.android.com/apk/res/android" android:shape="rectangle">
    <solid android:color="#E0E0E0"/>
    <corners android:radius="16dp"/>
    <stroke android:width="4dp" android:color="#CCCCCC"/>
</shape>
```

**`res/drawable/ic_target_icon.xml`** — File > New > Vector Asset → `android` 로봇 아이콘 → 이름 `ic_target_icon`

**`res/values/overlayable.xml`** — ★ RRO 계약

```xml
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <overlayable name="RROTargetTheme">
        <policy type="public">
            <item type="string"   name="hello_world"/>
            <item type="color"    name="background_color"/>
            <item type="color"    name="text_color"/>
            <item type="drawable" name="background_shape"/>
            <item type="drawable" name="ic_target_icon"/>
        </policy>
    </overlayable>
</resources>
```

Build > Generate App Bundles or APKs > Generate APKs → `app-debug.apk` 를 `C:\aosp16\bin\rrotarget.apk` 로 복사.

## Step 2. 오버레이 앱 RROOverlay

| 항목 | 값 |
|---|---|
| Name / Package | `RROOverlay` / `com.example.rrooverlay` · Java · minSdk 34 |

`MainActivity.java` 와 layout 은 **삭제**한다 (코드 없는 패키지).

**`AndroidManifest.xml`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android">
    <application android:hasCode="false" android:label="RRO Overlay"/>
    <overlay
        android:targetPackage="com.example.rrotarget"
        android:targetName="RROTargetTheme"
        android:priority="1"
        android:isStatic="false"/>
</manifest>
```

**`res/values/strings.xml`**

```xml
<resources>
    <string name="app_name">RROOverlay</string>
    <string name="hello_world">Hello, RRO on Android 16! 🚀</string>
</resources>
```

**`res/values/colors.xml`**

```xml
<resources>
    <color name="background_color">#FF4A148C</color>
    <color name="text_color">#FFF3E5F5</color>
</resources>
```

**`res/drawable/background_shape.xml`** — 타겟과 **같은 파일명**

```xml
<?xml version="1.0" encoding="utf-8"?>
<shape xmlns:android="http://schemas.android.com/apk/res/android" android:shape="rectangle">
    <gradient android:angle="90" android:startColor="#B39DDB" android:endColor="#673AB7"/>
    <corners android:radius="32dp"/>
</shape>
```

**`res/drawable/ic_target_icon.xml`** — Vector Asset → `rocket_launch` → 이름 `ic_target_icon`

`build.gradle.kts` 의 `dependencies` 는 비워도 된다. Generate APKs → `C:\aosp16\bin\rrooverlay.apk`.

## Step 3. 설치 · 활성화

```bat
adb install C:\aosp16\bin\rrotarget.apk
adb install C:\aosp16\bin\rrooverlay.apk
```

```bash
adb shell
cmd overlay list | grep -A2 com.example.rrotarget
# com.example.rrotarget
# [ ] com.example.rrooverlay          ← 매칭 성공 · 비활성

cmd overlay enable com.example.rrooverlay
cmd overlay list | grep rrooverlay
# [x] com.example.rrooverlay

am force-stop com.example.rrotarget
am start -n com.example.rrotarget/.MainActivity
```

화면: 보라 그라데이션 배경 · 로켓 아이콘 · "Hello, RRO on Android 16! 🚀" (연보라 글자).

```bash
cmd overlay dump com.example.rrooverlay | head -20
# mPackageName: com.example.rrooverlay  mTargetPackageName: com.example.rrotarget
# mTargetOverlayableName: RROTargetTheme  mState: 3 (STATE_ENABLED)  mPriority: 1

cmd overlay disable com.example.rrooverlay
am force-stop com.example.rrotarget; am start -n com.example.rrotarget/.MainActivity   # 원래대로
exit
```

## Step 4. 계약 실험

`RROTarget` 의 `overlayable.xml` 에서 `<item type="string" name="hello_world"/>` 를 지우고 재빌드·재설치(`adb install -r`) → 오버레이를 다시 `enable` → **문자열만 그대로**("how are you"), 색·drawable 은 바뀐다. 타겟이 허용하지 않은 리소스는 무시된다.

---

## 확인 포인트

- [ ] `cmd overlay list` 에 `[ ]` → `enable` 후 `[x]`
- [ ] 문자열·색상·배경·아이콘 5가지 모두 변경
- [ ] `overlayable` 에서 뺀 리소스는 안 바뀜

## 핵심 정리

| 요소 | 역할 |
|---|---|
| `<overlayable name><policy type="public">` | 타겟이 "이 리소스는 덮어써도 된다" 고 선언 (Android 11+ 의무) |
| `<overlay targetPackage targetName priority isStatic>` | RRO 가 "저 타겟의 저 계약을 덮는다" · priority 높은 것이 이김 · `isStatic=true` 면 항상 켜짐 |
| `hasCode="false"` | 리소스만 있는 APK |
| `cmd overlay` | `OverlayManagerService` 명령 — `list` 의 `[x]`/`[ ]`/`---` |
| 같은 이름·타입 | 덮어쓰기는 **리소스 이름**으로 매칭. 파일명·타입이 다르면 무시 |

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `INSTALL_FAILED_OLDER_SDK` | minSdk 34 |
| `---` 로 표시 | `targetName` 과 `overlayable name` 불일치 |
| enable 해도 안 바뀜 | 앱을 `force-stop` 후 재시작 · 리소스 이름 오타 · `overlayable` 누락 |
| `SecurityException` on enable | `adb shell` 이 root/shell 인지 (`adb root`) |
