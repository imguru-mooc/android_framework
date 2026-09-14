# 실습 12. Content Provider 메모장 — 멀티프로세스 데이터 공유 (선택 / 과제)

> **소요시간:** 45분 · **난이도:** ★★★ · **챕터:** Ch 3 Framework 내장 IPC
> **환경:** Windows 11 + Android Studio + Emulator (API 36)

## 목표

- `ContentProvider` 로 구조화된 데이터를 Process 간 CRUD 공유한다.
- `ContentResolver` 내부의 Binder IPC 와 `CursorWindow`(공유 메모리) 동작을 확인한다.
- `ContentObserver` 로 변경 알림을 받고, `adb shell content` 로 외부에서 접근한다.

> ⚠ AIDL 파일이 필요 없다. ContentProvider 는 Framework 내장 Binder IPC 를 사용한다.

---

## Step 1. 프로젝트 생성

| 항목 | 값 |
|---|---|
| Name | `MemoProvider` |
| Package | `com.example.memoprovider` |
| Language | **Java** / Minimum SDK API 28 이상 |

## Step 2. Contract

**`MemoContract.java`**

```java
package com.example.memoprovider;

import android.net.Uri;

/** Provider 와 Client 가 공유하는 상수. URI: content://com.example.memoprovider.memo/memos */
public final class MemoContract {
    public static final String AUTHORITY = "com.example.memoprovider.memo";
    public static final Uri CONTENT_URI = Uri.parse("content://" + AUTHORITY + "/memos");

    public static final String TABLE_NAME = "memos";
    public static final String COL_ID = "_id";
    public static final String COL_TITLE = "title";
    public static final String COL_CONTENT = "content";
    public static final String COL_CREATED_AT = "created_at";

    public static final String CONTENT_TYPE_DIR  = "vnd.android.cursor.dir/vnd." + AUTHORITY + ".memo";
    public static final String CONTENT_TYPE_ITEM = "vnd.android.cursor.item/vnd." + AUTHORITY + ".memo";

    private MemoContract() {}
}
```

## Step 3. DB Helper

**`MemoDbHelper.java`**

```java
package com.example.memoprovider;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

public class MemoDbHelper extends SQLiteOpenHelper {
    public MemoDbHelper(Context c) { super(c, "memo.db", null, 1); }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL("CREATE TABLE " + MemoContract.TABLE_NAME + " ("
                + MemoContract.COL_ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
                + MemoContract.COL_TITLE + " TEXT NOT NULL, "
                + MemoContract.COL_CONTENT + " TEXT, "
                + MemoContract.COL_CREATED_AT + " INTEGER)");
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int o, int n) {
        db.execSQL("DROP TABLE IF EXISTS " + MemoContract.TABLE_NAME);
        onCreate(db);
    }
}
```

## Step 4. Provider

**`MemoProvider.java`**

```java
package com.example.memoprovider;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import android.os.Binder;
import android.util.Log;

public class MemoProvider extends ContentProvider {
    private static final String TAG = "MemoProvider";
    private static final int MEMOS = 1, MEMO_ID = 2;
    private static final UriMatcher matcher = new UriMatcher(UriMatcher.NO_MATCH);
    static {
        matcher.addURI(MemoContract.AUTHORITY, "memos", MEMOS);
        matcher.addURI(MemoContract.AUTHORITY, "memos/#", MEMO_ID);
    }
    private MemoDbHelper helper;

    @Override public boolean onCreate() {
        helper = new MemoDbHelper(getContext());
        Log.d(TAG, "onCreate PID=" + android.os.Process.myPid());
        return true;
    }

    private void logCaller(String op) {
        Log.d(TAG, op + " from uid=" + Binder.getCallingUid() + " pid=" + Binder.getCallingPid()
                + " on " + Thread.currentThread().getName());
    }

    @Override
    public Cursor query(Uri uri, String[] proj, String sel, String[] args, String order) {
        logCaller("query");
        SQLiteDatabase db = helper.getReadableDatabase();
        if (matcher.match(uri) == MEMO_ID) {
            sel = MemoContract.COL_ID + "=?";
            args = new String[]{ String.valueOf(ContentUris.parseId(uri)) };
        }
        Cursor c = db.query(MemoContract.TABLE_NAME, proj, sel, args, null, null,
                order == null ? MemoContract.COL_CREATED_AT + " DESC" : order);
        c.setNotificationUri(getContext().getContentResolver(), uri);   // ★ Observer 연결
        return c;   // ★ CursorWindow (공유 메모리 FD) 로 Client 에 전달
    }

    @Override
    public Uri insert(Uri uri, ContentValues v) {
        logCaller("insert");
        if (!v.containsKey(MemoContract.COL_CREATED_AT))
            v.put(MemoContract.COL_CREATED_AT, System.currentTimeMillis());
        long id = helper.getWritableDatabase().insert(MemoContract.TABLE_NAME, null, v);
        Uri result = ContentUris.withAppendedId(MemoContract.CONTENT_URI, id);
        getContext().getContentResolver().notifyChange(uri, null);       // ★ Observer 알림
        return result;
    }

    @Override
    public int update(Uri uri, ContentValues v, String sel, String[] args) {
        logCaller("update");
        if (matcher.match(uri) == MEMO_ID) {
            sel = MemoContract.COL_ID + "=?";
            args = new String[]{ String.valueOf(ContentUris.parseId(uri)) };
        }
        int n = helper.getWritableDatabase().update(MemoContract.TABLE_NAME, v, sel, args);
        if (n > 0) getContext().getContentResolver().notifyChange(uri, null);
        return n;
    }

    @Override
    public int delete(Uri uri, String sel, String[] args) {
        logCaller("delete");
        if (matcher.match(uri) == MEMO_ID) {
            sel = MemoContract.COL_ID + "=?";
            args = new String[]{ String.valueOf(ContentUris.parseId(uri)) };
        }
        int n = helper.getWritableDatabase().delete(MemoContract.TABLE_NAME, sel, args);
        if (n > 0) getContext().getContentResolver().notifyChange(uri, null);
        return n;
    }

    @Override
    public String getType(Uri uri) {
        return matcher.match(uri) == MEMO_ID ? MemoContract.CONTENT_TYPE_ITEM : MemoContract.CONTENT_TYPE_DIR;
    }
}
```

## Step 5. AndroidManifest

```xml
<provider
    android:name=".MemoProvider"
    android:authorities="com.example.memoprovider.memo"
    android:exported="true"
    android:process=":provider_remote" />
```

> `android:process` 로 Provider 를 별도 Process 에 두어 Activity ↔ Provider 사이에 실제 Binder IPC 가 발생하게 한다. (실무에서는 보통 같은 Process)

## Step 6. Client Activity

**`res/layout/activity_main.xml`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<LinearLayout xmlns:android="http://schemas.android.com/apk/res/android"
    android:layout_width="match_parent" android:layout_height="match_parent"
    android:orientation="vertical" android:padding="16dp">
    <EditText android:id="@+id/title" android:layout_width="match_parent"
        android:layout_height="wrap_content" android:hint="제목"/>
    <EditText android:id="@+id/content" android:layout_width="match_parent"
        android:layout_height="wrap_content" android:hint="내용"/>
    <LinearLayout android:layout_width="match_parent" android:layout_height="wrap_content"
        android:orientation="horizontal">
        <Button android:id="@+id/btnInsert" android:layout_width="0dp" android:layout_weight="1"
            android:layout_height="wrap_content" android:text="추가"/>
        <Button android:id="@+id/btnQuery" android:layout_width="0dp" android:layout_weight="1"
            android:layout_height="wrap_content" android:text="조회"/>
        <Button android:id="@+id/btnDeleteAll" android:layout_width="0dp" android:layout_weight="1"
            android:layout_height="wrap_content" android:text="전체 삭제"/>
    </LinearLayout>
    <TextView android:id="@+id/logText" android:layout_width="match_parent"
        android:layout_height="match_parent" android:fontFamily="monospace" android:textSize="12sp"/>
</LinearLayout>
```

**`MainActivity.java`**

```java
package com.example.memoprovider;

import android.content.ContentValues;
import android.database.ContentObserver;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.net.Uri;
import android.widget.EditText;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {
    private TextView logText;
    private EditText title, content;

    // ★ 데이터 변경 시 Binder 콜백으로 호출 (Handler 지정 → Main Thread)
    private final ContentObserver observer = new ContentObserver(new Handler(Looper.getMainLooper())) {
        @Override public void onChange(boolean selfChange, Uri uri) {
            logText.append("🔔 onChange: " + uri + "\n");
            query();
        }
    };

    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        setContentView(R.layout.activity_main);
        logText = findViewById(R.id.logText);
        title = findViewById(R.id.title);
        content = findViewById(R.id.content);

        findViewById(R.id.btnInsert).setOnClickListener(v -> {
            ContentValues cv = new ContentValues();
            cv.put(MemoContract.COL_TITLE, title.getText().toString());
            cv.put(MemoContract.COL_CONTENT, content.getText().toString());
            Uri u = getContentResolver().insert(MemoContract.CONTENT_URI, cv);   // ★ Binder IPC
            logText.append("insert → " + u + "\n");
        });
        findViewById(R.id.btnQuery).setOnClickListener(v -> query());
        findViewById(R.id.btnDeleteAll).setOnClickListener(v ->
                logText.append("delete → " + getContentResolver().delete(MemoContract.CONTENT_URI, null, null) + "건\n"));

        getContentResolver().registerContentObserver(MemoContract.CONTENT_URI, true, observer);
    }

    private void query() {
        StringBuilder sb = new StringBuilder("─── memos ───\n");
        try (Cursor c = getContentResolver().query(MemoContract.CONTENT_URI, null, null, null, null)) {
            while (c != null && c.moveToNext()) {
                sb.append(c.getLong(0)).append(" | ").append(c.getString(1))
                  .append(" | ").append(c.getString(2)).append("\n");
            }
        }
        logText.append(sb.toString());
    }

    @Override
    protected void onDestroy() {
        getContentResolver().unregisterContentObserver(observer);
        super.onDestroy();
    }
}
```

## Step 7. adb 로 외부 접근

```bash
adb shell content insert --uri content://com.example.memoprovider.memo/memos \
    --bind title:s:"adb에서" --bind content:s:"직접 insert"

adb shell content query --uri content://com.example.memoprovider.memo/memos

adb shell content delete --uri content://com.example.memoprovider.memo/memos/1
```

`adb shell content` 명령을 실행하면 앱 화면의 Observer 가 즉시 반응한다.

```bash
adb shell "ps -A | grep memoprovider"
adb logcat -s MemoProvider
```

---

## 🎯 핵심 학습 정리

| 개념 | 확인 |
|---|---|
| ContentResolver → Binder | Provider 로그의 caller uid/pid 가 Activity Process 와 다름 |
| CursorWindow | 큰 결과도 FD(ashmem) 공유 메모리로 전달 — Transaction 한계 회피 |
| ContentObserver | `notifyChange()` → Binder 콜백 → Handler 로 Main Thread 전달 |
| `adb shell content` | 쉘 Process(uid 2000 또는 0) 에서의 Binder 호출 |
