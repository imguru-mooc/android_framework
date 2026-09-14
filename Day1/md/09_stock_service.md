# 실습 9. 비동기 주식 시세 Service — oneway + Callback

> **소요시간:** 40분 · **난이도:** ★★★ · **챕터:** Ch 2 AIDL
> **환경:** Windows 11 + Android Studio + Emulator (API 36)

## 목표

- 동기 vs 비동기(`oneway`) Binder IPC의 차이를 직접 체험한다.
- Service → Client 역방향 Callback 인터페이스를 구현한다.
- `RemoteCallbackList` 로 Client 사망 시 콜백을 자동 정리한다.

| 개념 | 체험 |
|---|---|
| 동기 IPC (`getPrice`) | Service가 2초 블로킹 → Client도 2초 대기 |
| 비동기 IPC (`subscribe`, oneway) | 요청 후 **0 ms** 에 즉시 반환 |
| Callback 패턴 | Service → Client 역방향 Binder IPC |
| RemoteCallbackList | Client 사망 시 자동 정리 |

---

## Step 1. 프로젝트 생성

| 항목 | 값 |
|---|---|
| Name | `StockLab` |
| Package | `com.example.stocklab` |
| Language | **Java** |
| Minimum SDK | API 28 이상 |

## Step 2. AIDL 활성화

`app/build.gradle.kts` → `android { buildFeatures { aidl = true } }` → Sync.

## Step 3. AIDL 파일

`app/src/main/aidl/com/example/stocklab/`

**`IStockCallback.aidl`**

```java
package com.example.stocklab;

// Service → Client 콜백. oneway: Service 가 콜백 호출 시 블로킹되지 않음
interface IStockCallback {
    oneway void onPriceUpdate(String symbol, double price, long timestamp);
    oneway void onError(String symbol, String message);
}
```

**`IStockService.aidl`**

```java
package com.example.stocklab;

import com.example.stocklab.IStockCallback;

interface IStockService {
    // 동기: 결과가 올 때까지 Client 블로킹 (2초 시뮬레이션)
    double getPrice(String symbol);

    // 비동기 (oneway): 즉시 반환, 결과는 콜백으로 수신
    oneway void subscribe(String symbol, IStockCallback callback);
    oneway void unsubscribe(String symbol);
}
```


## Step 4. Service 구현

**`StockService.java`**

```java
package com.example.stocklab;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.Process;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.SystemClock;
import android.util.Log;

import java.util.Map;
import java.util.Random;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

public class StockService extends Service {
    private static final String TAG = "StockService";

    // ★ Client 사망 시 자동으로 제거되는 콜백 목록 (내부적으로 DeathRecipient 사용)
    private final RemoteCallbackList<IStockCallback> callbacks = new RemoteCallbackList<>();
    private final Map<String, ScheduledFuture<?>> tasks = new ConcurrentHashMap<>();
    private final ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(2);
    private final Random random = new Random();

    private double basePrice(String symbol) {
        switch (symbol) {
            case "AAPL": return 185.50;
            case "GOOG": return 140.25;
            case "TSLA": return 245.80;
            default:     return 100.00;
        }
    }

    private final IStockService.Stub binder = new IStockService.Stub() {

        @Override
        public double getPrice(String symbol) {
            Log.d(TAG, "getPrice(" + symbol + ") — SYNC call, blocking 2 seconds... thread="
                    + Thread.currentThread().getName());
            SystemClock.sleep(2000);           // 느린 서버 시뮬레이션
            double p = basePrice(symbol);
            Log.d(TAG, "getPrice(" + symbol + ") — returning $" + p);
            return p;
        }

        @Override
        public void subscribe(String symbol, IStockCallback callback) {
            Log.d(TAG, "subscribe(" + symbol + ") — ASYNC, returning immediately!");
            callbacks.register(callback, symbol);   // cookie 로 symbol 저장
            Log.d(TAG, "subscribe(" + symbol + ") — callback registered");

            if (tasks.containsKey(symbol)) return;
            ScheduledFuture<?> f = scheduler.scheduleAtFixedRate(
                    () -> broadcast(symbol), 1, 1, TimeUnit.SECONDS);
            tasks.put(symbol, f);
        }

        @Override
        public void unsubscribe(String symbol) {
            Log.d(TAG, "unsubscribe(" + symbol + ")");
            ScheduledFuture<?> f = tasks.remove(symbol);
            if (f != null) f.cancel(true);
        }
    };

    // ★ 등록된 모든 Client 에게 역방향 Binder IPC
    private void broadcast(String symbol) {
        double price = basePrice(symbol) + (random.nextDouble() - 0.5) * 4;
        long now = System.currentTimeMillis();

        int n = callbacks.beginBroadcast();
        for (int i = 0; i < n; i++) {
            if (!symbol.equals(callbacks.getBroadcastCookie(i))) continue;
            try {
                callbacks.getBroadcastItem(i).onPriceUpdate(symbol, price, now);
            } catch (RemoteException e) {
                Log.w(TAG, "callback failed (client died?)", e);
            }
        }
        callbacks.finishBroadcast();
    }

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d(TAG, "══════════════════════════════════");
        Log.d(TAG, "StockService CREATED (PID: " + Process.myPid() + ")");
        Log.d(TAG, "══════════════════════════════════");
    }

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "onBind() — client connected!");
        return binder;
    }

    @Override
    public void onDestroy() {
        scheduler.shutdownNow();
        callbacks.kill();
        super.onDestroy();
    }
}
```

## Step 5. 레이아웃

**`res/layout/activity_main.xml`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<LinearLayout xmlns:android="http://schemas.android.com/apk/res/android"
    android:layout_width="match_parent"
    android:layout_height="match_parent"
    android:orientation="vertical"
    android:padding="24dp">

    <TextView android:layout_width="wrap_content" android:layout_height="wrap_content"
        android:text="📈 Stock Service Lab" android:textSize="22sp" android:textStyle="bold"/>
    <TextView android:layout_width="wrap_content" android:layout_height="wrap_content"
        android:text="oneway + Callback 비동기 IPC 실습" android:textSize="13sp"
        android:textColor="#888" android:layout_marginBottom="12dp"/>

    <TextView android:id="@+id/statusText" android:layout_width="match_parent"
        android:layout_height="wrap_content" android:text="⏳ StockService 연결 중..."
        android:textSize="12sp" android:background="#F0F0F0" android:padding="10dp"
        android:layout_marginBottom="12dp"/>

    <Button android:id="@+id/btnSync" android:layout_width="match_parent"
        android:layout_height="wrap_content" android:text="동기 호출 getPrice(AAPL) — 2초 블로킹"/>
    <Button android:id="@+id/btnSubscribe" android:layout_width="match_parent"
        android:layout_height="wrap_content" android:text="비동기 subscribe(AAPL) — oneway"/>
    <Button android:id="@+id/btnUnsubscribe" android:layout_width="match_parent"
        android:layout_height="wrap_content" android:text="unsubscribe(AAPL)"/>

    <ScrollView android:id="@+id/scrollView" android:layout_width="match_parent"
        android:layout_height="0dp" android:layout_weight="1" android:layout_marginTop="12dp">
        <TextView android:id="@+id/logText" android:layout_width="match_parent"
            android:layout_height="wrap_content" android:textSize="13sp"
            android:fontFamily="monospace" android:padding="8dp" android:background="#FAFAFA"/>
    </ScrollView>
</LinearLayout>
```

## Step 6. Client Activity

**`MainActivity.java`**

```java
package com.example.stocklab;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "StockClient";
    private IStockService service;
    private TextView statusText, logText;
    private ScrollView scrollView;
    private final SimpleDateFormat fmt = new SimpleDateFormat("HH:mm:ss.SSS", Locale.US);

    // ★ Service → Client 콜백. Binder Thread 에서 호출된다.
    private final IStockCallback.Stub callback = new IStockCallback.Stub() {
        @Override
        public void onPriceUpdate(String symbol, double price, long timestamp) {
            String t = Thread.currentThread().getName();
            runOnUiThread(() -> appendLog(String.format(Locale.US,
                    "  ← %s $%.2f  (%s) [thread=%s]", symbol, price, fmt.format(new Date(timestamp)), t)));
        }
        @Override
        public void onError(String symbol, String message) {
            runOnUiThread(() -> appendLog("  ← ❌ " + symbol + ": " + message));
        }
    };

    private final IBinder.DeathRecipient deathRecipient = () -> {
        Log.e(TAG, "binderDied() — thread=" + Thread.currentThread().getName());
        runOnUiThread(() -> statusText.setText("☠ StockService 사망 감지 (binderDied)"));
    };

    private final ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            service = IStockService.Stub.asInterface(binder);
            try { binder.linkToDeath(deathRecipient, 0); } catch (RemoteException ignored) {}
            statusText.setText("✅ StockService 연결됨 (앱 PID " + android.os.Process.myPid() + ")");
            appendLog("Service 연결 완료");
        }
        @Override
        public void onServiceDisconnected(ComponentName name) {
            service = null;
            appendLog("❌ onServiceDisconnected (Main Thread)");
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        statusText = findViewById(R.id.statusText);
        logText = findViewById(R.id.logText);
        scrollView = findViewById(R.id.scrollView);

        findViewById(R.id.btnSync).setOnClickListener(v -> syncCall());
        findViewById(R.id.btnSubscribe).setOnClickListener(v -> asyncCall());
        findViewById(R.id.btnUnsubscribe).setOnClickListener(v -> {
            try { if (service != null) service.unsubscribe("AAPL"); appendLog("→ unsubscribe(AAPL)"); }
            catch (RemoteException e) { appendLog("오류: " + e); }
        });

        Intent intent = new Intent("com.example.stocklab.STOCK_SERVICE");
        intent.setPackage(getPackageName());
        bindService(intent, connection, Context.BIND_AUTO_CREATE);
    }

    private void syncCall() {
        if (service == null) return;
        appendLog("→ getPrice(AAPL) 동기 호출... (UI 가 멈추는지 관찰)");
        long t0 = System.currentTimeMillis();
        try {
            double p = service.getPrice("AAPL");     // ★ Main Thread 블로킹 — 실무에서는 금지
            appendLog(String.format(Locale.US, "  ← $%.2f  소요 %d ms", p, System.currentTimeMillis() - t0));
        } catch (RemoteException e) {
            appendLog("오류: " + e);
        }
    }

    private void asyncCall() {
        if (service == null) return;
        long t0 = System.currentTimeMillis();
        try {
            service.subscribe("AAPL", callback);     // ★ oneway — 즉시 반환
            appendLog("→ subscribe(AAPL) oneway 반환 소요 " + (System.currentTimeMillis() - t0) + " ms");
        } catch (RemoteException e) {
            appendLog("오류: " + e);
        }
    }

    private void appendLog(String msg) {
        logText.append(msg + "\n");
        scrollView.post(() -> scrollView.fullScroll(ScrollView.FOCUS_DOWN));
    }

    @Override
    protected void onDestroy() {
        try { if (service != null) service.unsubscribe("AAPL"); } catch (RemoteException ignored) {}
        unbindService(connection);
        super.onDestroy();
    }
}
```

## Step 7. AndroidManifest

```xml
<service
    android:name=".StockService"
    android:exported="true"
    android:process=":stock_remote">
    <intent-filter>
        <action android:name="com.example.stocklab.STOCK_SERVICE" />
    </intent-filter>
</service>
```

## Step 8. 테스트 시나리오

| 테스트 | 기대 결과 |
|---|---|
| 동기 호출 버튼 | UI 가 2초 멈춤 → `소요 2000 ms` |
| 비동기 subscribe | `oneway 반환 소요 0 ms` → 1초마다 시세 콜백 수신 |
| 콜백 로그의 thread | `Binder:xxxx_N` — Binder Thread 에서 호출됨 |
| `adb shell "ps -A \| grep stocklab"` | 앱 / `:stock_remote` 두 Process |
| `kill -9 <stock_remote PID>` | `binderDied` 가 `onServiceDisconnected` 보다 먼저 |

```bash
adb logcat -s StockService StockClient
```

---

## 🎯 핵심 학습 정리

| 관찰 | Binder 개념 |
|---|---|
| 동기 2000 ms vs 비동기 0 ms | `oneway` 는 Driver 에서 즉시 반환 — reply 대기 없음 |
| 구독 후 자동 업데이트 | Service → Client 역방향 Binder IPC (콜백) |
| 콜백에서 `runOnUiThread` 필수 | 콜백은 **Binder Thread** 에서 호출됨 |
| `binderDied()` 가 먼저 | Driver 직접 감지 (Binder Thread) vs AMS 경유 (Main Thread) |
| Client 앱 강제 종료 후 Service 로그 | `RemoteCallbackList` 가 죽은 콜백을 자동 제거 |
