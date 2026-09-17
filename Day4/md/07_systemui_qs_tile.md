# 실습 7. SystemUI 소스 수정 — CpuInfoTile QS 타일 추가

> **소요시간:** 40분 · **난이도:** ★★★ · **챕터:** Ch 2 · 저장소 SystemUI 실습 3 (실습 4 `BatteryTemperatureView` 는 참고 코드)
> **전제:** 각 계정에서 `m SystemUI` 가 사전 1회 빌드됨 (증분 빌드 수 분)

## 목표

- `QSTileImpl` 을 상속한 타일을 만들어 Dagger 로 등록하고 기본 타일 목록에 넣는다.
- `m SystemUI` 로 APK 만 다시 빌드해 교체한다 — 이미지 리빌드 없이.
- 등록을 빼고 빌드해 **크래시 로그를 읽어 원인을 찾는** 절차를 익힌다.

## 소스 구조

```
frameworks/base/packages/SystemUI/
├── src/com/android/systemui/qs/tiles/            ← 타일 구현 (WifiTile.java …)
├── src/com/android/systemui/qs/tiles/di/ 또는 dagger/   ← @Binds @IntoMap 등록 (grep 으로 위치 확인)
├── res/drawable/                                 ← 아이콘
├── res/values/config.xml                         ← quick_settings_tiles_default
└── res/values/strings.xml
```

---

## Step 1. 타일 클래스

**`frameworks/base/packages/SystemUI/src/com/android/systemui/qs/tiles/CpuInfoTile.java`**

```java
package com.android.systemui.qs.tiles;

import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.service.quicksettings.Tile;

import com.android.internal.logging.MetricsLogger;
import com.android.systemui.res.R;                      // Android 15+ 는 com.android.systemui.res.R (없으면 com.android.systemui.R)
import com.android.systemui.dagger.qualifiers.Background;
import com.android.systemui.dagger.qualifiers.Main;
import com.android.systemui.plugins.ActivityStarter;
import com.android.systemui.plugins.FalsingManager;
import com.android.systemui.plugins.qs.QSTile.BooleanState;
import com.android.systemui.plugins.statusbar.StatusBarStateController;
import com.android.systemui.qs.QSHost;
import com.android.systemui.qs.QsEventLogger;
import com.android.systemui.qs.logging.QSLogger;
import com.android.systemui.qs.tileimpl.QSTileImpl;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

import javax.inject.Inject;

public class CpuInfoTile extends QSTileImpl<BooleanState> {
    public static final String TILE_SPEC = "cpuinfo";          // ★ config.xml 과 Dagger 키
    private boolean mShowingInfo = false;

    @Inject
    public CpuInfoTile(QSHost host, QsEventLogger uiEventLogger,
                       @Background Looper backgroundLooper, @Main Handler mainHandler,
                       FalsingManager falsingManager, MetricsLogger metricsLogger,
                       StatusBarStateController statusBarStateController,
                       ActivityStarter activityStarter, QSLogger qsLogger) {
        super(host, uiEventLogger, backgroundLooper, mainHandler, falsingManager,
              metricsLogger, statusBarStateController, activityStarter, qsLogger);
    }

    @Override public BooleanState newTileState() {
        BooleanState s = new BooleanState(); s.handlesLongClick = false; return s;
    }

    @Override protected void handleClick(android.view.View view) {   // 시그니처가 다르면 상위 클래스에 맞춘다
        mShowingInfo = !mShowingInfo;
        refreshState();
    }

    @Override protected void handleUpdateState(BooleanState state, Object arg) {
        state.label = "CPU Info";
        state.icon = ResourceIcon.get(R.drawable.ic_qs_cpu_info);
        state.state = mShowingInfo ? Tile.STATE_ACTIVE : Tile.STATE_INACTIVE;
        state.secondaryLabel = mShowingInfo ? readLoadAvg() : "Tap to show";
    }

    @Override public int getMetricsCategory() { return 0; }
    @Override public Intent getLongClickIntent() { return new Intent(android.provider.Settings.ACTION_DEVICE_INFO_SETTINGS); }
    @Override public CharSequence getTileLabel() { return "CPU Info"; }

    private String readLoadAvg() {
        try (BufferedReader r = new BufferedReader(new FileReader("/proc/loadavg"))) {
            String line = r.readLine();                              // "0.52 0.48 0.40 1/812 6120"
            return line != null ? "load " + line.split(" ")[0] : "N/A";
        } catch (IOException e) { return "N/A"; }
    }
}
```

## Step 2. 아이콘 · 문자열

**`res/drawable/ic_qs_cpu_info.xml`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<vector xmlns:android="http://schemas.android.com/apk/res/android"
    android:width="24dp" android:height="24dp" android:viewportWidth="24" android:viewportHeight="24"
    android:tint="?android:attr/colorControlNormal">
    <path android:fillColor="#FFFFFFFF"
        android:pathData="M15,21h-2v-2h2V21z M13,14h-2v5h2V14z M21,12h-2v4h2V12z M17,14h-2v3h2V14z M7,14H5v5h2V14z M9,12H7v6h2V12z M11,10H9v8h2V10z M19,10h-2v7h2V10z M3,14H1v5h2V14z M15,7h-2v2h2V7z M11,3H9v2h2V3z M13,3h-2v6h2V3z M5,10H3v3h2V10z"/>
</vector>
```

## Step 3. Dagger 등록 — 기존 타일을 따라 한다

```bash
cd ~/android/frameworks/base/packages/SystemUI
grep -rn "StringKey(FlashlightTile.TILE_SPEC)" src/ | head -3
# src/com/android/systemui/qs/tiles/di/…  또는  src/com/android/systemui/dagger/QSModule.java
```

찾은 파일의 `FlashlightTile` 등록 바로 아래에 같은 형태로 추가:

```java
/** */
@Binds
@IntoMap
@StringKey(CpuInfoTile.TILE_SPEC)
QSTileImpl<?> bindCpuInfoTile(CpuInfoTile cpuInfoTile);
```

(`import com.android.systemui.qs.tiles.CpuInfoTile;` 추가.)

> 구버전(Android 13 이하)은 `QSTileHost.createTileInternal()` 의 `case "cpuinfo":` 방식이다. 저장소 문서의 방식과 비교해 보라.

## Step 4. 기본 타일 목록

**`res/values/config.xml`** 의 `quick_settings_tiles_default` 끝에 `,cpuinfo`:

```xml
<string name="quick_settings_tiles_default" translatable="false">
    wifi,bt,dnd,flashlight,rotation,battery,cell,airplane,cpuinfo
</string>
```

Car SystemUI 가 자체 config 를 갖는다면 `packages/apps/Car/SystemUI/res/values/config.xml` 도 확인한다 (`grep -rn quick_settings_tiles_default packages/apps/Car/SystemUI/res`).

## Step 5. 빌드 · 교체

```bash
cd ~/android
m SystemUI                       # Car 타겟은 CarSystemUI 모듈이 함께 갱신된다
ls -la $OUT/system_ext/priv-app/CarSystemUI/CarSystemUI.apk
```

**WinSCP** — `system_ext/priv-app/CarSystemUI/CarSystemUI.apk` → `C:\aosp16\bin\`

```bat
adb root
adb remount
adb push C:\aosp16\bin\CarSystemUI.apk /system_ext/priv-app/CarSystemUI/CarSystemUI.apk
```

```bash
adb shell
killall com.android.systemui
sleep 5; pidof com.android.systemui          # 새 PID 가 나오면 정상 기동
exit
```

Emulator 에서 상단바를 내려 QS 패널 → **CPU Info** 타일 → 탭하면 `load 0.52` 표시.

```bash
adb shell dumpsys statusbar | grep -i cpuinfo
```

## Step 6. 실패 재현 — 등록을 빼고 빌드

Step 3 의 `@Binds` 블록을 주석 처리 → `m SystemUI` → push → `killall`. SystemUI 가 반복 크래시하거나 타일이 없다.

```bash
adb shell
logcat -d -s AndroidRuntime | grep -B2 -A12 "systemui" | head -40
# … IllegalStateException / Dagger … "cpuinfo" … 또는 QSFactoryImpl: No tile for spec cpuinfo
exit
```

메시지에서 `cpuinfo` 를 찾으면 원인이 등록 누락이라는 것을 안다. 주석을 풀고 재빌드·교체.

---

## 확인 포인트

- [ ] QS 패널에 CPU Info 타일, 탭 시 `load …`
- [ ] `m SystemUI` 증분 빌드 수 분, APK 만 교체로 반영
- [ ] 등록 누락 시 로그에서 `cpuinfo` 를 찾아 진단

## 핵심 정리

| 항목 | 내용 |
|---|---|
| `QSTileImpl<State>` | `newTileState` · `handleClick` · `handleUpdateState` — 상태를 바꾸면 `refreshState()` |
| Dagger `@Binds @IntoMap @StringKey` | spec 문자열 → 타일 클래스 맵. `QSFactoryImpl` 이 이 맵으로 생성 |
| `config.xml` | 기본 타일 순서. RRO 로도 바꿀 수 있다 (실습 6 의 방식) |
| 배포 | `m SystemUI` → APK push → `killall` — 이미지 리빌드 불필요 (델타 배포표) |

참고 코드 (시간 내 진행하지 않음) — 상태바에 커스텀 뷰: `statusbar/BatteryTemperatureView.java` (`TextView` 상속, `onAttachedToWindow` 에서 `Handler.postDelayed` 10초 갱신, `BatteryManager.BATTERY_PROPERTY_TEMPERATURE`) 를 `res/layout/status_bar.xml` 배터리 아이콘 앞에 배치. "우리 회사 상태를 상태바에" 요청의 표준 패턴. 저장소 SystemUI 실습 4 참조.

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `package com.android.systemui.res.R does not exist` | `com.android.systemui.R` 로 |
| `handleClick` 시그니처 오류 | 상위 `QSTileImpl` 의 선언에 맞춘다 (`@Nullable View` 유무) |
| `killall` 후 SystemUI 안 뜸 | `logcat -s AndroidRuntime` · `adb reboot` |
| 타일 목록에 없음 | Car SystemUI 의 `config.xml` 이 우선 — 그 파일에 추가 |
