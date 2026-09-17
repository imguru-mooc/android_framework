# 실습 4. ListOverlays 도구 · RRO 우선순위 충돌 · SRO 1회

> **소요시간:** 20분 · **난이도:** ★★☆ · **챕터:** Ch 1
> **디렉토리:** `~/android/RRO_test/overlay_list/` · 저장소 `RRO_test/overlay_list`, `SRO_product_flavors_android16.md`

## 목표

- `IOverlayManager` hidden API 를 `app_process` 로 직접 호출하는 도구를 빌드해 오버레이 상태를 출력한다 (Day 3 실습 8 의 app_process + Day 2 의 hidden API).
- RRO 두 개가 같은 리소스를 덮을 때 **priority** 가 어떻게 작용하는지 본다.
- SRO(Product Flavors)를 5분에 한 번 돌려 "빌드 시 vs 런타임" 을 정리한다.

---

## Step 1. ListOverlays (빌드 서버)

**`RRO_test/overlay_list/ListOverlays.java`**

```java
import android.content.om.IOverlayManager;
import android.content.om.OverlayInfo;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import java.util.List;

public class ListOverlays {
    public static void main(String[] args) {
        String targetPkg = (args.length > 0 && !args[0].isEmpty()) ? args[0] : "com.android.systemui";

        // ★ hidden API — SDK 에는 없다. Framework 안(app_process)에서만 가능
        IOverlayManager om = IOverlayManager.Stub.asInterface(ServiceManager.getService("overlay"));
        if (om == null) { System.err.println("OverlayManager service not found!"); return; }

        try {
            List<OverlayInfo> overlays = om.getOverlayInfosForTarget(targetPkg, UserHandle.myUserId());
            System.out.println("Overlays for " + targetPkg + " (" + overlays.size() + ")");
            for (OverlayInfo oi : overlays) {
                System.out.printf("%-50s : %s  priority=%d  state=%s%n",
                        oi.packageName, oi.isEnabled() ? "ENABLED" : "disabled", oi.priority,
                        OverlayInfo.stateToString(oi.state));
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }
}
```

**`RRO_test/overlay_list/Android.bp`**

```text
java_binary {
    name: "list_overlays",
    srcs: ["ListOverlays.java"],
    libs: ["framework", "services"],        // hidden API 접근
    main_class: "ListOverlays",
    sdk_version: "core_platform",
}
```

> `Android.bp` 를 새로 만들었으므로 Soong 이 한 번 재분석한다 (5~6분). 강사가 실습 1 에서 미리 만들어 두었으면 바로 빌드된다.

```bash
cd ~/android/RRO_test/overlay_list
mm
ls $OUT/system/framework/list_overlays.jar
```

## Step 2. 실행 (Windows)

**WinSCP** — `system/framework/list_overlays.jar` → `C:\aosp16\bin\`

```bat
adb push C:\aosp16\bin\list_overlays.jar /data
```

```bash
adb shell
CLASSPATH=/data/list_overlays.jar app_process /system/bin ListOverlays
# Overlays for com.android.systemui (12)
# com.android.systemui.car.rro                       : ENABLED  priority=…  state=STATE_ENABLED
# com.android.systemui.auto_generated_rro_vendor__   : disabled …
# com.android.car.ui.overlay.sdk.carsystemui          : disabled  state=STATE_NO_IDMAP   ← cmd overlay 의 ---
CLASSPATH=/data/list_overlays.jar app_process /system/bin ListOverlays com.example.rrotarget
exit
```

`cmd overlay list` 와 같은 정보지만, **우리 코드가 Framework 서비스를 직접 부른** 것이다. `app_process` 가 VM 을 띄우고(Day 3 실습 8) `ServiceManager.getService("overlay")` 로 Binder Proxy 를 얻어(Day 1·2) `IOverlayManager` AIDL 을 호출한다.

## Step 3. 우선순위 충돌

실습 3 의 `RROOverlay` 프로젝트를 복사해 **`RROOverlay2`** (`com.example.rrooverlay2`) 를 만든다. 바꿀 것은 두 가지:

```xml
<!-- AndroidManifest.xml -->
<overlay android:targetPackage="com.example.rrotarget" android:targetName="RROTargetTheme"
         android:priority="2" android:isStatic="false"/>
```

```xml
<!-- res/values/strings.xml -->
<string name="hello_world">Priority 2 wins</string>
```

```bat
adb install C:\aosp16\bin\rrooverlay2.apk
```

```bash
adb shell
cmd overlay enable com.example.rrooverlay
cmd overlay enable com.example.rrooverlay2
am force-stop com.example.rrotarget; am start -n com.example.rrotarget/.MainActivity
# 화면: "Priority 2 wins"  (배경·아이콘은 overlay1 의 것 — overlay2 에는 없으니 아래 것이 보인다)

CLASSPATH=/data/list_overlays.jar app_process /system/bin ListOverlays com.example.rrotarget
# com.example.rrooverlay   : ENABLED  priority=1
# com.example.rrooverlay2  : ENABLED  priority=2     ← 높은 priority 가 나중에 적용 = 이긴다

cmd overlay disable com.example.rrooverlay2
exit
```

리소스마다 "가장 높은 priority 의 활성 오버레이" 가 이긴다. 현장에서 "RRO 를 넣었는데 안 바뀐다" 의 흔한 원인.

## Step 4. SRO — Product Flavors 5분

`RROTarget` 의 `app/build.gradle.kts` `android {}` 안에:

```kotlin
flavorDimensions += "theme"
productFlavors {
    create("original") { dimension = "theme" }
    create("themed")   { dimension = "theme" }
}
```

`app/src/themed/res/values/colors.xml` 을 만든다 (덮어쓸 것만):

```xml
<resources>
    <color name="background_color">#FF1B5E20</color>
    <color name="text_color">#FFC8E6C9</color>
</resources>
```

Build > Select Build Variant > `themedDebug` → Run → 초록 배경. `originalDebug` 로 되돌리면 원래대로.

| | SRO (빌드 시) | RRO (런타임) |
|---|---|---|
| 결정 시점 | APK 빌드 | 설치 후 `cmd overlay enable` |
| 되돌리기 | 재빌드 | `disable` 즉시 |
| 용도 | White Label · 고객사별 APK | 테마 · SystemUI 커스터마이징 |

---

## 확인 포인트

- [ ] `ListOverlays` 출력에 `state=STATE_NO_IDMAP` 항목(= `---`) 이 보임
- [ ] priority 2 오버레이의 문자열이 이김
- [ ] `themedDebug` 로 색 변경, main 리소스 수정 없음

## 핵심 정리

- hidden API 는 SDK 에서 막혀 있지만 `app_process` + `libs: ["framework"]` 로 빌드하면 쓸 수 있다 — 진단 도구는 이렇게 만든다.
- priority 는 **높을수록 우선**. 같은 리소스를 두 RRO 가 덮으면 `cmd overlay dump` 의 priority 로 확인.
- SRO 는 App 개발 영역, RRO 는 Framework/제품 영역.
