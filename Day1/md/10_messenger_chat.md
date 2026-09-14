# 실습 10. Messenger 채팅 앱 — 경량 IPC

> **소요시간:** 30분 · **난이도:** ★★☆ · **챕터:** Ch 3 Framework 내장 IPC
> **환경:** Windows 11 + Android Studio + Emulator (API 36)

## 목표

- AIDL 없이 `Messenger` 로 Process 간 양방향 통신을 구현한다.
- Handler 단일 Thread 순차 처리와 `replyTo` 패턴을 이해한다.

| 개념 | 체험 |
|---|---|
| Messenger IPC | `.aidl` 파일 없이 Process 간 통신 |
| Handler 기반 처리 | 단일 Thread 순차 처리 → 자동 Thread 안전 |
| `replyTo` | Service → Client 역방향 메시지 |
| `Message` + `Bundle` | 약한 타입 데이터 전달 |

---

## Step 1. 프로젝트 생성

| 항목 | 값 |
|---|---|
| Name | `MessengerChat` |
| Package | `com.example.messengerchat` |
| Language | **Java** |
| Minimum SDK | API 28 이상 |

> ⚠ 이 실습은 **AIDL 파일을 만들지 않는다.** `buildFeatures.aidl` 도 불필요하다. Messenger는 Framework 내장 IPC다.

## Step 2. 메시지 상수

**`ChatConstants.java`**

```java
package com.example.messengerchat;

/** Messenger IPC 는 Method 이름이 없고 Message.what 정수로 요청을 구분한다. */
public final class ChatConstants {
    // Client → Service
    public static final int MSG_REGISTER    = 1;
    public static final int MSG_UNREGISTER  = 2;
    public static final int MSG_SEND_TEXT   = 3;
    public static final int MSG_GET_TIME    = 4;

    // Service → Client
    public static final int MSG_REPLY       = 100;
    public static final int MSG_BROADCAST   = 101;
    public static final int MSG_TIME        = 102;

    public static final String KEY_TEXT  = "text";
    public static final String KEY_COUNT = "count";
    public static final String KEY_CLIENTS = "clients";

    private ChatConstants() {}
}
```

## Step 3. Service

**`ChatService.java`**

```java
package com.example.messengerchat;

import android.app.Service;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

/** ★ AIDL 파일 불필요 — Messenger 가 내부적으로 Binder 를 사용한다. */
public class ChatService extends Service {
    private static final String TAG = "ChatService";

    private final List<Messenger> clients = new ArrayList<>();
    private int messageCount = 0;

    /** ★ 모든 메시지를 단일 Thread(Main Looper)에서 순차 처리한다. AIDL 은 여러 Binder Thread 에서 동시 호출된다. */
    class IncomingHandler extends Handler {
        IncomingHandler(Looper looper) { super(looper); }

        @Override
        public void handleMessage(Message msg) {
            Log.d(TAG, "what=" + msg.what + " from PID=" + android.os.Binder.getCallingPid()
                    + " on " + Thread.currentThread().getName());
            switch (msg.what) {
                case ChatConstants.MSG_REGISTER:
                    if (msg.replyTo != null) {
                        clients.add(msg.replyTo);
                        sendText(msg.replyTo, ChatConstants.MSG_REPLY,
                                "👋 서버에 연결되었습니다! (등록 클라이언트: " + clients.size() + "명)");
                    }
                    break;

                case ChatConstants.MSG_UNREGISTER:
                    if (msg.replyTo != null) clients.remove(msg.replyTo);
                    break;

                case ChatConstants.MSG_SEND_TEXT: {
                    String text = msg.getData().getString(ChatConstants.KEY_TEXT, "");
                    messageCount++;
                    if (msg.replyTo != null)
                        sendText(msg.replyTo, ChatConstants.MSG_REPLY, "✅ Echo: " + text.toUpperCase());
                    broadcast("📢 [서버] 새 메시지: " + text);
                    break;
                }

                case ChatConstants.MSG_GET_TIME: {
                    Message reply = Message.obtain(null, ChatConstants.MSG_TIME);
                    Bundle b = new Bundle();
                    b.putString(ChatConstants.KEY_TEXT,
                            new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US).format(new Date()));
                    b.putInt(ChatConstants.KEY_COUNT, messageCount);
                    b.putInt(ChatConstants.KEY_CLIENTS, clients.size());
                    reply.setData(b);
                    send(msg.replyTo, reply);
                    break;
                }

                default:
                    super.handleMessage(msg);
            }
        }
    }

    private void sendText(Messenger to, int what, String text) {
        Message m = Message.obtain(null, what);
        Bundle b = new Bundle();
        b.putString(ChatConstants.KEY_TEXT, text);
        m.setData(b);
        send(to, m);
    }

    private void send(Messenger to, Message m) {
        if (to == null) return;
        try { to.send(m); }
        catch (RemoteException e) { Log.w(TAG, "client dead — removing"); clients.remove(to); }
    }

    private void broadcast(String text) {
        for (Messenger c : new ArrayList<>(clients))
            sendText(c, ChatConstants.MSG_BROADCAST, text);
    }

    // ★ Messenger 가 Handler 를 감싸 IBinder 를 만들어 준다.
    private final Messenger messenger = new Messenger(new IncomingHandler(Looper.getMainLooper()));

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "onBind — PID " + android.os.Process.myPid());
        return messenger.getBinder();
    }
}
```

## Step 4. 레이아웃

**`res/layout/activity_main.xml`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<LinearLayout xmlns:android="http://schemas.android.com/apk/res/android"
    android:layout_width="match_parent" android:layout_height="match_parent"
    android:orientation="vertical" android:padding="16dp">

    <EditText android:id="@+id/input" android:layout_width="match_parent"
        android:layout_height="wrap_content" android:hint="메시지 입력" android:text="Hello Android!"/>

    <LinearLayout android:layout_width="match_parent" android:layout_height="wrap_content"
        android:orientation="horizontal">
        <Button android:id="@+id/btnSend" android:layout_width="0dp" android:layout_weight="1"
            android:layout_height="wrap_content" android:text="전송"/>
        <Button android:id="@+id/btnTime" android:layout_width="0dp" android:layout_weight="1"
            android:layout_height="wrap_content" android:text="서버 시간 요청"/>
        <Button android:id="@+id/btnBurst" android:layout_width="0dp" android:layout_weight="1"
            android:layout_height="wrap_content" android:text="연속 전송 (5개)"/>
    </LinearLayout>

    <ScrollView android:id="@+id/scrollView" android:layout_width="match_parent"
        android:layout_height="0dp" android:layout_weight="1" android:layout_marginTop="8dp">
        <TextView android:id="@+id/logText" android:layout_width="match_parent"
            android:layout_height="wrap_content" android:fontFamily="monospace"
            android:textSize="13sp" android:padding="8dp" android:background="#FAFAFA"/>
    </ScrollView>
</LinearLayout>
```

## Step 5. Client Activity

**`MainActivity.java`**

```java
package com.example.messengerchat;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    private Messenger serviceMessenger;          // Service 로 보내는 통로
    private Messenger clientMessenger;           // Service 가 답장하는 통로 (replyTo)
    private TextView logText;
    private ScrollView scrollView;
    private EditText input;

    /** Service → Client 메시지 수신 (Main Looper 이므로 UI 직접 갱신 가능) */
    class ReplyHandler extends Handler {
        ReplyHandler() { super(Looper.getMainLooper()); }
        @Override
        public void handleMessage(Message msg) {
            Bundle b = msg.getData();
            switch (msg.what) {
                case ChatConstants.MSG_REPLY:
                case ChatConstants.MSG_BROADCAST:
                    appendLog("  ← " + b.getString(ChatConstants.KEY_TEXT)); break;
                case ChatConstants.MSG_TIME:
                    appendLog("  ← 🕐 서버 시간: " + b.getString(ChatConstants.KEY_TEXT)
                            + "\n       메시지 수: " + b.getInt(ChatConstants.KEY_COUNT)
                            + " | 클라이언트: " + b.getInt(ChatConstants.KEY_CLIENTS)); break;
                default: super.handleMessage(msg);
            }
        }
    }

    private final ServiceConnection connection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            serviceMessenger = new Messenger(binder);   // ★ IBinder → Messenger
            appendLog("✅ 서비스 연결 완료!");
            Message reg = Message.obtain(null, ChatConstants.MSG_REGISTER);
            reg.replyTo = clientMessenger;
            send(reg);
            appendLog("📋 서비스에 클라이언트 등록 완료");
        }
        @Override
        public void onServiceDisconnected(ComponentName name) {
            serviceMessenger = null;
            appendLog("❌ 서비스 연결 해제됨");
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        logText = findViewById(R.id.logText);
        scrollView = findViewById(R.id.scrollView);
        input = findViewById(R.id.input);
        clientMessenger = new Messenger(new ReplyHandler());

        findViewById(R.id.btnSend).setOnClickListener(v -> sendText(input.getText().toString()));
        findViewById(R.id.btnTime).setOnClickListener(v -> {
            Message m = Message.obtain(null, ChatConstants.MSG_GET_TIME);
            m.replyTo = clientMessenger;
            send(m);
            appendLog("→ [요청] 서버 시간 조회");
        });
        findViewById(R.id.btnBurst).setOnClickListener(v -> burst());

        appendLog("═══ Messenger Chat Lab 시작 ═══");
        Intent intent = new Intent("com.example.messengerchat.CHAT_SERVICE");
        intent.setPackage(getPackageName());
        appendLog("bindService() 반환: " + bindService(intent, connection, Context.BIND_AUTO_CREATE));
    }

    private void sendText(String text) {
        Message m = Message.obtain(null, ChatConstants.MSG_SEND_TEXT);
        Bundle b = new Bundle();
        b.putString(ChatConstants.KEY_TEXT, text);
        m.setData(b);
        m.replyTo = clientMessenger;
        long t0 = System.currentTimeMillis();
        send(m);
        appendLog("→ [전송] \"" + text + "\" (소요: " + (System.currentTimeMillis() - t0) + "ms)");
    }

    private void burst() {
        appendLog("═══ 연속 5개 메시지 전송 ═══");
        String[] msgs = {"Hello!", "How are you?", "Binder IPC", "Messenger 테스트", "완료!"};
        for (int i = 0; i < msgs.length; i++) {
            sendText(msgs[i]);
        }
        appendLog("★ Handler 큐에 순서대로 처리됨 (단일 스레드!)");
    }

    private void send(Message m) {
        if (serviceMessenger == null) { appendLog("❌ 미연결"); return; }
        try { serviceMessenger.send(m); }
        catch (RemoteException e) { appendLog("❌ 전송 실패: " + e.getMessage()); }
    }

    private void appendLog(String msg) {
        logText.append(msg + "\n");
        scrollView.post(() -> scrollView.fullScroll(ScrollView.FOCUS_DOWN));
    }

    @Override
    protected void onDestroy() {
        if (serviceMessenger != null) {
            Message m = Message.obtain(null, ChatConstants.MSG_UNREGISTER);
            m.replyTo = clientMessenger;
            send(m);
            unbindService(connection);
        }
        super.onDestroy();
    }
}
```

## Step 6. AndroidManifest

```xml
<service
    android:name=".ChatService"
    android:exported="true"
    android:process=":chat_remote">
    <intent-filter>
        <action android:name="com.example.messengerchat.CHAT_SERVICE" />
    </intent-filter>
</service>
```

## Step 7. 테스트 시나리오

| 테스트 | 기대 결과 |
|---|---|
| 전송 | Echo 응답 + 브로드캐스트 두 줄 수신 |
| 서버 시간 요청 | Service 상태(메시지 수, 클라이언트 수)가 Bundle 로 도착 |
| 연속 전송 5개 | 응답이 **전송 순서 그대로** 도착 (Handler 단일 Thread) |
| `adb shell "ps -A \| grep messenger"` | 앱 / `:chat_remote` 두 Process |
| `kill -9 <chat_remote PID>` | `onServiceDisconnected` → `BIND_AUTO_CREATE` 로 자동 재시작 |

---

## 🎯 핵심 학습 정리

| Messenger | AIDL |
|---|---|
| `.aidl` 불필요, `Message.what` 으로 구분 | 인터페이스 정의, 타입 안전한 Method |
| Handler 단일 Thread 순차 처리 | Binder Thread Pool 병렬 처리 |
| 응답은 `replyTo` 로 비동기 | 동기 반환값 / oneway 선택 |
| 간단한 명령·알림에 적합 | 복잡한 API, 성능이 중요한 경우 |

둘 다 같은 Binder 위에서 동작한다. 차이는 개발자가 Proxy/Stub 을 직접 다루느냐, Framework 가 `Handler` 로 감싸 주느냐다.
