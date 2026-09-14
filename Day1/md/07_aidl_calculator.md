# 실습 7. AIDL 계산기 — 기본 IPC Service 구현

> **소요시간:** 45분 · **난이도:** ★★☆ · **챕터:** Ch 2 AIDL
> **환경:** Windows 11 + Android Studio + Emulator (API 36)

## 목표

- AIDL 인터페이스를 정의하고 Service / Client를 구현한다.
- `android:process` 로 Service를 별도 Process에 띄워 **실제 Binder IPC**를 발생시킨다.
- `getCallingUid()/Pid()`, `clearCallingIdentity()` 패턴을 적용한다.

---

## Step 1. 프로젝트 생성

Android Studio → **New Project → Empty Views Activity**

| 항목 | 값 |
|---|---|
| Name | `BinderLab` |
| Package | `com.example.binderlab` |
| Language | **Java** |
| Minimum SDK | API 36 |
| Build configuration | Kotlin DSL |

## Step 2. AIDL 활성화

**`app/build.gradle.kts`** 의 `android { }` 안에 추가:

```kotlin
buildFeatures {
    aidl = true
}
```

**Sync Now** 클릭.

## Step 3. AIDL 인터페이스 정의

`app` 우클릭 → **New → Folder → AIDL Folder** → `app/src/main/aidl/com/example/binderlab/`

**`ICalculatorService.aidl`**

```java
package com.example.binderlab;

interface ICalculatorService {
    int add(int a, int b);
    int subtract(int a, int b);
    int multiply(int a, int b);
    double divide(int a, int b);

    // 호출자 정보 반환 (Binder 보안 확인용)
    String getCallerInfo();

    // 계산 이력 조회
    List<String> getHistory();
}
```


## Step 4. Service 구현

**`CalculatorService.java`**

```java
package com.example.binderlab;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.os.Process;
import android.os.RemoteException;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

public class CalculatorService extends Service {
    private static final String TAG = "CalcService";
    private final List<String> history = new ArrayList<>();
    private final Object lock = new Object();

    // ★ Stub 구현 = Binder 객체. 각 Method는 Binder Thread에서 실행된다.
    private final ICalculatorService.Stub binder = new ICalculatorService.Stub() {

        @Override
        public int add(int a, int b) {
            int r = a + b;
            log("add", a, b, r);
            return r;
        }

        @Override
        public int subtract(int a, int b) {
            int r = a - b;
            log("subtract", a, b, r);
            return r;
        }

        @Override
        public int multiply(int a, int b) {
            int r = a * b;
            log("multiply", a, b, r);
            return r;
        }

        @Override
        public double divide(int a, int b) throws RemoteException {
            if (b == 0) {
                // AIDL 은 RemoteException 계열만 전달 가능. IllegalArgumentException 은 Client 로 그대로 전달된다.
                throw new IllegalArgumentException("divide by zero");
            }
            double r = (double) a / b;
            log("divide", a, b, r);
            return r;
        }

        @Override
        public String getCallerInfo() {
            int callerPid = Binder.getCallingPid();
            int callerUid = Binder.getCallingUid();
            int myPid = Process.myPid();
            return "Caller PID: " + callerPid
                    + "\nCaller UID: " + callerUid
                    + "\nService PID: " + myPid
                    + "\nBinder Thread: " + Thread.currentThread().getName()
                    + "\nSame process: " + (callerPid == myPid ? "YES" : "NO");
        }

        @Override
        public List<String> getHistory() {
            // ★ clearCallingIdentity 패턴 — 이후 내부 호출은 Service 자신의 UID 로 수행
            long token = Binder.clearCallingIdentity();
            try {
                synchronized (lock) {
                    return new ArrayList<>(history);
                }
            } finally {
                Binder.restoreCallingIdentity(token);
            }
        }
    };

    private void log(String op, int a, int b, double r) {
        String entry = op + "(" + a + ", " + b + ") = " + r
                + "  [uid=" + Binder.getCallingUid() + " pid=" + Binder.getCallingPid() + "]";
        Log.d(TAG, entry + " on " + Thread.currentThread().getName());
        synchronized (lock) {
            history.add(entry);
            if (history.size() > 50) history.remove(0);
        }
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "Service created! PID=" + Process.myPid());
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "Service bound!");
        return binder;
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "Service destroyed!");
        super.onDestroy();
    }
}
```

## Step 5. AndroidManifest 등록

`<application>` 안에 추가:

```xml
<service
    android:name=".CalculatorService"
    android:exported="true"
    android:process=":calc_remote">
    <intent-filter>
        <action android:name="com.example.binderlab.CALCULATOR" />
    </intent-filter>
</service>
```

> **핵심:** `android:process=":calc_remote"` — Service를 **별도 Process**에서 실행해 실제 Binder IPC를 강제한다.

## Step 6. 레이아웃

**`res/layout/activity_main.xml`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<LinearLayout xmlns:android="http://schemas.android.com/apk/res/android"
    android:layout_width="match_parent"
    android:layout_height="match_parent"
    android:orientation="vertical"
    android:padding="16dp">

    <TextView android:id="@+id/infoText"
        android:layout_width="match_parent" android:layout_height="wrap_content"
        android:text="연결 중..." android:textSize="12sp"
        android:background="#F0F0F0" android:padding="8dp" android:layout_marginBottom="12dp"/>

    <LinearLayout android:layout_width="match_parent" android:layout_height="wrap_content"
        android:orientation="horizontal" android:layout_marginBottom="8dp">
        <EditText android:id="@+id/inputA" android:layout_width="0dp" android:layout_weight="1"
            android:layout_height="wrap_content" android:inputType="number" android:hint="A" android:text="42"/>
        <EditText android:id="@+id/inputB" android:layout_width="0dp" android:layout_weight="1"
            android:layout_height="wrap_content" android:inputType="number" android:hint="B" android:text="7"
            android:layout_marginStart="8dp"/>
    </LinearLayout>

    <LinearLayout android:layout_width="match_parent" android:layout_height="wrap_content"
        android:orientation="horizontal" android:layout_marginBottom="12dp">
        <Button android:id="@+id/btnAdd" android:layout_width="0dp" android:layout_weight="1"
            android:layout_height="wrap_content" android:text="+" />
        <Button android:id="@+id/btnSub" android:layout_width="0dp" android:layout_weight="1"
            android:layout_height="wrap_content" android:text="-" />
        <Button android:id="@+id/btnMul" android:layout_width="0dp" android:layout_weight="1"
            android:layout_height="wrap_content" android:text="×" />
        <Button android:id="@+id/btnDiv" android:layout_width="0dp" android:layout_weight="1"
            android:layout_height="wrap_content" android:text="÷" />
    </LinearLayout>

    <Button android:id="@+id/btnHistory" android:layout_width="match_parent"
        android:layout_height="wrap_content" android:text="📜 계산 이력 보기" android:layout_marginBottom="16dp"/>

    <ScrollView android:layout_width="match_parent" android:layout_height="0dp" android:layout_weight="1">
        <TextView android:id="@+id/resultText" android:layout_width="match_parent"
            android:layout_height="wrap_content" android:text="결과가 여기에 표시됩니다."
            android:textSize="16sp" android:padding="12dp" android:background="#FAFAFA"/>
    </ScrollView>
</LinearLayout>
```

## Step 7. Client Activity

**`MainActivity.java`**

```java
package com.example.binderlab;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import java.util.List;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "CalcClient";
    private ICalculatorService service;
    private boolean bound = false;
    private TextView resultText, infoText;
    private EditText inputA, inputB;

    private final ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            // ★ IBinder → AIDL Proxy 변환
            service = ICalculatorService.Stub.asInterface(binder);
            bound = true;
            Log.d(TAG, "Service connected!");
            try {
                infoText.setText(service.getCallerInfo());
            } catch (RemoteException e) {
                infoText.setText("getCallerInfo 실패: " + e);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.w(TAG, "Service disconnected!");
            bound = false;
            service = null;
            infoText.setText("❌ 연결 끊김");
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        resultText = findViewById(R.id.resultText);
        infoText = findViewById(R.id.infoText);
        inputA = findViewById(R.id.inputA);
        inputB = findViewById(R.id.inputB);

        findViewById(R.id.btnAdd).setOnClickListener(v -> calc('+'));
        findViewById(R.id.btnSub).setOnClickListener(v -> calc('-'));
        findViewById(R.id.btnMul).setOnClickListener(v -> calc('*'));
        findViewById(R.id.btnDiv).setOnClickListener(v -> calc('/'));
        findViewById(R.id.btnHistory).setOnClickListener(v -> showHistory());

        Intent intent = new Intent("com.example.binderlab.CALCULATOR");
        intent.setPackage(getPackageName());   // 명시적 Intent 필요 (Android 5.0+)
        bindService(intent, connection, Context.BIND_AUTO_CREATE);
    }

    private void calc(char op) {
        if (!bound) { Toast.makeText(this, "Service 미연결", Toast.LENGTH_SHORT).show(); return; }
        try {
            int a = Integer.parseInt(inputA.getText().toString());
            int b = Integer.parseInt(inputB.getText().toString());
            long t0 = System.nanoTime();
            double r;
            switch (op) {
                case '+': r = service.add(a, b); break;
                case '-': r = service.subtract(a, b); break;
                case '*': r = service.multiply(a, b); break;
                default:  r = service.divide(a, b); break;
            }
            long us = (System.nanoTime() - t0) / 1000;
            resultText.setText(a + " " + op + " " + b + " = " + r + "\n(IPC " + us + " µs)");
        } catch (RemoteException e) {
            resultText.setText("RemoteException: " + e.getMessage());
        } catch (Exception e) {
            resultText.setText("오류: " + e.getMessage());
        }
    }

    private void showHistory() {
        if (!bound) return;
        try {
            List<String> h = service.getHistory();
            resultText.setText(h.isEmpty() ? "(이력 없음)" : String.join("\n", h));
        } catch (RemoteException e) {
            resultText.setText("RemoteException: " + e.getMessage());
        }
    }

    @Override
    protected void onDestroy() {
        if (bound) { unbindService(connection); bound = false; }
        super.onDestroy();
    }
}
```

## Step 8. 실행 및 검증

**Run → Run 'app'**

```bash
# 메인 앱과 Service가 별도 Process인지
adb shell "ps -A | grep binderlab"
# u0_a150  12345  1000  com.example.binderlab
# u0_a150  12346  1000  com.example.binderlab:calc_remote

adb shell dumpsys activity services | grep -A 5 "CalculatorService"

adb logcat -s CalcService CalcClient
```

---

## 🎯 핵심 학습 포인트

1. 화면 상단 `getCallerInfo()` 결과에서 **"Same process: NO"** 확인 → 실제 IPC 발생
2. Manifest에서 `android:process` 를 제거하고 재실행 → **"Same process: YES"** → 같은 Process면 Proxy 없이 직접 호출
3. Logcat에서 Service Method가 **`Binder:12346_2`** 같은 Binder Thread에서 실행되는 것 확인
4. `divide(1, 0)` → Service의 예외가 Client에 그대로 전달되는 것 확인

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `cannot find symbol ICalculatorService` | `buildFeatures.aidl = true` 확인 후 Sync Now |
| `bindService` 가 false 반환 | `intent.setPackage()` 누락 또는 Manifest action 불일치 |
| Service 연결 후 바로 끊김 | Logcat 에서 `calc_remote` Process 크래시 확인 |
