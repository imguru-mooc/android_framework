# 실습 12. Android App 에서 Native Service 호출하기 (선택 / 과제)

> **소요시간:** 30분 · **난이도:** ★★★ · **챕터:** Ch 3 System Service
> **환경:** Android Studio + Emulator (실습 8 `led_auth_service` 실행 중)

## 목표

- Java App 이 **AIDL Java backend 없이** `IBinder.transact()` 로 C++ Service 를 직접 호출한다.
- `ServiceManager.getService()` 가 hidden API 라 리플렉션으로 접근해야 함을 본다.
- App 의 UID(10xxx) 로 호출하면 실습 8 의 UID 검사에 걸려 `SecurityException` 이 되는 것을 확인한다.

---

## Step 1. 프로젝트

| 항목 | 값 |
|---|---|
| Name | `NativeCallLab` |
| Package | `com.example.nativecalllab` |
| Language | Java · Minimum SDK 28 이상 |

## Step 2. MainActivity

**`MainActivity.java`**

```java
package com.example.nativecalllab;

import android.os.Bundle;
import android.os.IBinder;
import android.os.Parcel;
import android.os.Process;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import java.lang.reflect.Method;

public class MainActivity extends AppCompatActivity {
    private static final String DESCRIPTOR = "ILedAuthService";           // 실습 8 AIDL 의 descriptor (패키지 없음)
    private static final int TRANSACTION_LEDON = IBinder.FIRST_CALL_TRANSACTION;   // = 1

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        TextView tv = new TextView(this);
        tv.setTextSize(14);
        tv.setPadding(32, 32, 32, 32);
        setContentView(tv);
        tv.setText(callLedAuth());
    }

    private String callLedAuth() {
        StringBuilder sb = new StringBuilder("App uid=" + Process.myUid() + " pid=" + Process.myPid() + "\n\n");
        try {
            // ★ android.os.ServiceManager 는 @hide — 리플렉션으로 접근
            Class<?> sm = Class.forName("android.os.ServiceManager");
            Method getService = sm.getMethod("getService", String.class);
            IBinder binder = (IBinder) getService.invoke(null, "led.auth");
            if (binder == null) return sb.append("led.auth not found — 서버가 떠 있나?").toString();

            sb.append("descriptor from server: ").append(binder.getInterfaceDescriptor()).append("\n");

            Parcel data = Parcel.obtain(), reply = Parcel.obtain();
            try {
                data.writeInterfaceToken(DESCRIPTOR);                      // CHECK_INTERFACE 통과용
                binder.transact(TRANSACTION_LEDON, data, reply, 0);        // ★ Proxy 없이 직접
                reply.readException();                                     // ★ Status(EX_SECURITY) → SecurityException
                sb.append("LEDON() succeeded");
            } finally {
                data.recycle(); reply.recycle();
            }
        } catch (SecurityException e) {
            sb.append("SecurityException: ").append(e.getMessage());       // uid 10xxx → 거부 (정상)
        } catch (Exception e) {
            sb.append("error: ").append(e);
        }
        return sb.toString();
    }
}
```

## Step 3. 실행

실습 8 서버가 떠 있는 상태에서 앱 실행:

```text
App uid=10152 pid=7210

descriptor from server: ILedAuthService
SecurityException: Only system UID may toggle LED
```

서버 로그: `LEDON denied  (uid=10152 pid=7210)`

## Step 4. 왜 거부되는가 — 그리고 진짜 App 은 어떻게 하나

- App 의 UID 는 10000 번대. 실습 8 의 서비스는 1000 만 허용 → 커널이 붙인 UID 로 정확히 거부된다.
- 실제 Framework 는 App 이 HAL/Native Service 를 **직접 부르지 못하게** 하고, `system_server` 의 Java Service 가 대신 호출한다 (App → AMS/LightsService (uid 1000) → HAL). 권한 검사는 Java Service 에서 `checkCallingPermission()` 으로 한다.
- 이 실습의 리플렉션 방식은 hidden API 라 Play 배포 앱에서는 차단(`NoSuchMethodError`)될 수 있다. 플랫폼 앱(platform 서명, `android:sharedUserId="android.uid.system"`)만 정식으로 쓸 수 있다.

## Step 5. (과제) system UID 로 실행해 보기

Emulator 의 platform 키로 서명하고 `sharedUserId="android.uid.system"` 을 추가하면 uid 1000 으로 실행되어 `LEDON() succeeded` 가 나온다. 서명 키는 `~/aosp/build/make/target/product/security/platform.pk8 / platform.x509.pem`. `apksigner` 로 재서명 후 `adb install`.

---

## 확인 포인트

- [ ] `binder.getInterfaceDescriptor()` 가 `ILedAuthService`
- [ ] `reply.readException()` 이 `SecurityException` 으로 변환
- [ ] 서버 로그의 uid 가 App uid 와 일치

## 핵심 정리

- Java `IBinder.transact()` + `Parcel` = C++ `remote()->transact()` + `Parcel`. 언어만 다르다.
- `binder::Status` 의 예외 코드는 Java 쪽 `Parcel.readException()` 에서 Java 예외로 되살아난다.
- App 은 Native Service 를 직접 부르지 않는다 — **system_server 를 거치는 것이 Android 의 보안 설계**다.
