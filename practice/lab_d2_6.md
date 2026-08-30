# [D2-6] 실습 6-2 — 커스텀 서비스 ②: Stub 구현 + 3대 보안 패턴

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★★☆ | Day 2 Ch 6 · ★전원 필수 | WinSCP + 서버 · 🎬 ⑬ 선시청 |

> 🎯 **실습 목표** — `DeviceInfoService extends Stub` 구현에 **enforce 권한 검사 / clearCallingIdentity·restore(finally) / ConcurrentHashMap** 3대 패턴을 정확히 심는다.

## Step 1. 파일 생성

```bash
mkdir -p ~/aosp/frameworks/base/services/core/java/com/android/server/custom/
```

🖱 WinSCP로 `DeviceInfoService.java` 생성 → 아래 골격 입력(핵심부):

```java
package com.android.server.custom;

import android.content.Context;
import android.os.Binder;
import android.os.Build;
import android.os.SystemClock;
import android.os.custom.IDeviceInfoService;
import android.util.Slog;
import java.util.concurrent.ConcurrentHashMap;

public class DeviceInfoService extends IDeviceInfoService.Stub {
    private static final String TAG = "DeviceInfoService";
    private static final String PERM_WRITE = "android.permission.WRITE_DEVICE_INFO";
    private final Context mContext;
    // ★ Binder Thread Pool 동시 접근 → ConcurrentHashMap
    private final ConcurrentHashMap<String, String> mProperties = new ConcurrentHashMap<>();

    public DeviceInfoService(Context context) {
        mContext = context;
        Slog.i(TAG, "DeviceInfoService initialized");
    }

    @Override public String getDeviceModel() { return Build.MODEL; }          // 읽기: 권한 불필요
    @Override public long getUptimeSeconds() { return SystemClock.elapsedRealtime() / 1000; }

    @Override public boolean setCustomProperty(String key, String value) {
        // ★ ① 검사 먼저 — 호출자 명찰 기준
        mContext.enforceCallingOrSelfPermission(PERM_WRITE, TAG);
        // ★ ② 내부 작업은 system 명찰로
        final long token = Binder.clearCallingIdentity();
        try {
            if (key == null || key.isEmpty()) return false;
            mProperties.put(key, value != null ? value : "");
            return true;
        } finally {
            // ★ ③ 어떤 경로로 나가든 반드시 원복
            Binder.restoreCallingIdentity(token);
        }
    }

    @Override public String getCustomProperty(String key) { return mProperties.get(key); }
}
```

## Step 2. 셀프 코드 리뷰 (체크 3문)

```text
Q1. enforce가 clearCallingIdentity보다 먼저인 이유는? (명찰이 지워지면 검사 무의미)
Q2. restore가 finally에 있는 이유는? (예외 시 스레드 신원 오염 방지 — 🎬 ⑬)
Q3. HashMap이 아니라 ConcurrentHashMap인 이유는? (Binder 스레드 풀 동시 진입)
```

✅ **예상 결과:** 3문을 자신의 말로 답하고 옆자리와 상호 검증

# 🏁 Pass 판정 체크리스트

- [ ] 파일이 정확한 패키지/경로에 존재
- [ ] 3대 패턴이 모두 코드에 존재 (① enforce ② clear/try ③ finally restore)
- [ ] 셀프 리뷰 3문 답변 완료

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| cannot find symbol IDeviceInfoService | 6-1 미반영/미빌드 | D2-5 Step 3 재확인 |
| restore를 try 안에 둠 | finally 의미 오해 | 반드시 finally 블록 — 리뷰 Q2 |

# 🚗 현업 활용 포인트

💡 AOSP 서비스 코드 리뷰에서 가장 먼저 훑는 곳이 정확히 이 3패턴입니다. `grep -rn clearCallingIdentity frameworks/base/services/core/java | wc -l` — 수백 건의 선배 코드가 같은 관용구를 씁니다.

---
*실습 D2-6 (17/36) · 다음: **D2-7 실습 6-3 Manager/등록***
