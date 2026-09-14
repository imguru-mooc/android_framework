# 실습 11. TransactionTooLargeException 재현 & 해결

> **소요시간:** 20분 · **난이도:** ★★☆ · **챕터:** Ch 1 Binder
> **환경:** Windows 11 + Android Studio + Emulator (API 36)

## 목표

- Binder Transaction Buffer(Process당 1 MB, 단일 Transaction 약 500 KB~1 MB) 한계를 직접 재현한다.
- `ParcelFileDescriptor`(Pipe)로 대용량 데이터를 안전하게 전달한다.

---

## Step 1. 프로젝트 생성

| 항목 | 값 |
|---|---|
| Name | `BinderLimitLab` |
| Package | `com.example.binderlimitlab` |
| Language | **Java** / Minimum SDK API 28 이상 |

`app/build.gradle.kts` → `buildFeatures { aidl = true }` → Sync.

## Step 2. AIDL

**`app/src/main/aidl/com/example/binderlimitlab/IDataService.aidl`**

```java
package com.example.binderlimitlab;

interface IDataService {
    // Binder 직접 전달 — 크기 한계 있음
    byte[] getDataDirect(int sizeBytes);

    // Pipe(FD) 전달 — 크기 제한 없음
    ParcelFileDescriptor getDataViaPipe(int sizeBytes);
}
```

## Step 3. Service

**`DataService.java`**

```java
package com.example.binderlimitlab;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;

public class DataService extends Service {
    private static final String TAG = "DataService";

    private final IDataService.Stub binder = new IDataService.Stub() {

        @Override
        public byte[] getDataDirect(int sizeBytes) {
            Log.d(TAG, "getDataDirect(" + sizeBytes + ")");
            byte[] data = new byte[sizeBytes];
            Arrays.fill(data, (byte) 'A');
            return data;             // ★ reply Parcel 에 통째로 복사 → 한계 초과 시 예외
        }

        @Override
        public ParcelFileDescriptor getDataViaPipe(int sizeBytes) {
            Log.d(TAG, "getDataViaPipe(" + sizeBytes + ")");
            try {
                ParcelFileDescriptor[] pipe = ParcelFileDescriptor.createPipe();
                ParcelFileDescriptor readEnd = pipe[0];
                ParcelFileDescriptor writeEnd = pipe[1];

                // ★ Binder Thread 를 막지 않도록 별도 Thread 에서 write
                new Thread(() -> {
                    try (FileOutputStream out = new FileOutputStream(writeEnd.getFileDescriptor())) {
                        byte[] chunk = new byte[64 * 1024];
                        Arrays.fill(chunk, (byte) 'B');
                        int remaining = sizeBytes;
                        while (remaining > 0) {
                            int n = Math.min(chunk.length, remaining);
                            out.write(chunk, 0, n);
                            remaining -= n;
                        }
                    } catch (IOException e) {
                        Log.e(TAG, "pipe write failed", e);
                    } finally {
                        try { writeEnd.close(); } catch (IOException ignored) {}
                    }
                }, "PipeWriter").start();

                return readEnd;      // ★ Parcel 에는 FD 하나만 실린다
            } catch (IOException e) {
                Log.e(TAG, "createPipe failed", e);
                return null;
            }
        }
    };

    @Override
    public IBinder onBind(Intent intent) { return binder; }
}
```

## Step 4. 레이아웃

**`res/layout/activity_main.xml`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<LinearLayout xmlns:android="http://schemas.android.com/apk/res/android"
    android:layout_width="match_parent" android:layout_height="match_parent"
    android:orientation="vertical" android:padding="16dp">

    <Button android:id="@+id/btnDirect" android:layout_width="match_parent"
        android:layout_height="wrap_content" android:text="Binder 직접 전달 (크기 증가 테스트)"/>
    <Button android:id="@+id/btnPipe" android:layout_width="match_parent"
        android:layout_height="wrap_content" android:text="Pipe/FD 전달 (4 MB)"/>
    <Button android:id="@+id/btnCompare" android:layout_width="match_parent"
        android:layout_height="wrap_content" android:text="속도 비교 (256 KB)"/>

    <ScrollView android:id="@+id/scrollView" android:layout_width="match_parent"
        android:layout_height="0dp" android:layout_weight="1" android:layout_marginTop="8dp">
        <TextView android:id="@+id/logText" android:layout_width="match_parent"
            android:layout_height="wrap_content" android:fontFamily="monospace"
            android:textSize="12sp" android:padding="8dp" android:background="#FAFAFA"/>
    </ScrollView>
</LinearLayout>
```

## Step 5. Client Activity

**`MainActivity.java`**

```java
package com.example.binderlimitlab;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.os.TransactionTooLargeException;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;

import java.io.FileInputStream;
import java.io.IOException;

public class MainActivity extends AppCompatActivity {
    private IDataService service;
    private TextView logText;
    private ScrollView scrollView;

    private final ServiceConnection connection = new ServiceConnection() {
        @Override public void onServiceConnected(ComponentName n, IBinder b) {
            service = IDataService.Stub.asInterface(b);
            appendLog("✅ DataService 연결");
        }
        @Override public void onServiceDisconnected(ComponentName n) { service = null; }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        logText = findViewById(R.id.logText);
        scrollView = findViewById(R.id.scrollView);

        findViewById(R.id.btnDirect).setOnClickListener(v -> new Thread(this::testDirect).start());
        findViewById(R.id.btnPipe).setOnClickListener(v -> new Thread(() -> testPipe(4 * 1024 * 1024)).start());
        findViewById(R.id.btnCompare).setOnClickListener(v -> new Thread(this::compare).start());

        Intent i = new Intent("com.example.binderlimitlab.DATA_SERVICE");
        i.setPackage(getPackageName());
        bindService(i, connection, Context.BIND_AUTO_CREATE);
    }

    /** 10KB → 2MB 까지 2배씩 키우며 한계 탐색 */
    private void testDirect() {
        appendLog("═══ Binder 직접 전달 테스트 ═══");
        int size = 10 * 1024;
        while (size <= 2 * 1024 * 1024) {
            try {
                long t0 = System.nanoTime();
                byte[] d = service.getDataDirect(size);
                appendLog(String.format("  ✅ %6d KB: 성공 (%d µs)", size / 1024, (System.nanoTime() - t0) / 1000));
            } catch (TransactionTooLargeException e) {
                appendLog(String.format("  ❌ %6d KB: TransactionTooLargeException!", size / 1024));
                appendLog("     → " + e.getMessage());
                break;
            } catch (RemoteException e) {
                appendLog("  ❌ RemoteException: " + e);
                break;
            }
            size *= 2;
        }
    }

    private long testPipe(int size) {
        long t0 = System.nanoTime();
        try (ParcelFileDescriptor pfd = service.getDataViaPipe(size);
             FileInputStream in = new FileInputStream(pfd.getFileDescriptor())) {
            byte[] buf = new byte[64 * 1024];
            long total = 0; int n;
            while ((n = in.read(buf)) > 0) total += n;
            long us = (System.nanoTime() - t0) / 1000;
            appendLog(String.format("  ✅ Pipe %d KB 수신 (%d µs)", total / 1024, us));
            return us;
        } catch (RemoteException | IOException e) {
            appendLog("  ❌ Pipe 실패: " + e);
            return -1;
        }
    }

    private void compare() {
        int size = 256 * 1024;
        appendLog("═══ 256 KB 속도 비교 ═══");
        long tDirect = -1;
        try {
            long t0 = System.nanoTime();
            service.getDataDirect(size);
            tDirect = (System.nanoTime() - t0) / 1000;
            appendLog("  Binder 직접: " + tDirect + " µs");
        } catch (RemoteException e) { appendLog("  Binder 직접 실패: " + e); }
        long tPipe = testPipe(size);
        if (tDirect > 0 && tPipe > 0)
            appendLog(tDirect < tPipe ? "  → 소용량은 Binder 직접 전달이 빠름"
                                       : "  → Pipe 가 더 빠르거나 비슷함");
        appendLog("  → 대용량 데이터는 항상 Pipe/FD 를 사용하세요");
    }

    private void appendLog(String msg) {
        runOnUiThread(() -> {
            logText.append(msg + "\n");
            scrollView.post(() -> scrollView.fullScroll(ScrollView.FOCUS_DOWN));
        });
    }

    @Override
    protected void onDestroy() { unbindService(connection); super.onDestroy(); }
}
```

## Step 6. AndroidManifest

```xml
<service
    android:name=".DataService"
    android:exported="true"
    android:process=":data_remote">
    <intent-filter>
        <action android:name="com.example.binderlimitlab.DATA_SERVICE" />
    </intent-filter>
</service>
```

## Step 7. 테스트

| 버튼 | 기대 결과 |
|---|---|
| Binder 직접 전달 | 10 KB … 320 KB 성공 → 640 KB 또는 1 MB 근처에서 `TransactionTooLargeException` |
| Pipe/FD 전달 (4 MB) | 4096 KB 정상 수신 |
| 속도 비교 | 소용량은 직접 전달이 약간 빠르거나 비슷 |

```bash
adb logcat -s DataService AndroidRuntime
```

---

## 🎯 핵심 학습 정리

- Binder Transaction Buffer는 Process당 **1 MB**를 모든 진행 중 Transaction이 **공유**한다. 단일 Transaction도 그 절반 정도가 실질 한계다.
- `Intent`, `Bundle`, `savedInstanceState` 도 같은 제한을 받는다 → 큰 Bitmap/List 전달 시 크래시의 주범.
- 대용량은 `ParcelFileDescriptor`(Pipe, ashmem/`SharedMemory`, 파일)로 **FD만 Parcel에 실어** 전달한다.
