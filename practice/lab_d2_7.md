# [D2-7] 실습 6-3 — 커스텀 서비스 ③: Manager · Context · SystemServer 등록

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★★☆ | Day 2 Ch 6 · ★전원 필수 | WinSCP + 서버 |

> 🎯 **실습 목표** — 앱측 포장지(**Manager**)와 창구(**Context 상수 + SystemServiceRegistry**), 그리고 탄생 코드(**SystemServer**)까지 — 🎬 ⑤의 ③④⑤번 파일을 완성한다.

## Step 1. DeviceInfoManager (앱측 포장지)

🖱 `frameworks/base/core/java/android/os/custom/DeviceInfoManager.java`:

```java
package android.os.custom;

import android.content.Context;
import android.os.RemoteException;

public class DeviceInfoManager {
    private final IDeviceInfoService mService;
    public DeviceInfoManager(Context ctx, IDeviceInfoService service) { mService = service; }

    public String getDeviceModel() {
        try { return mService.getDeviceModel(); }
        catch (RemoteException e) { throw e.rethrowFromSystemServer(); }
    }
    public long getUptimeSeconds() {
        try { return mService.getUptimeSeconds(); }
        catch (RemoteException e) { throw e.rethrowFromSystemServer(); }
    }
    // set/getCustomProperty 동일 패턴으로 래핑
}
```

❓ **왜 Manager?** 앱 개발자에게 RemoteException·Binder를 숨기는 것이 플랫폼 API의 예의 — CarPropertyManager와 같은 층입니다.

## Step 2. Context 상수 + Registry 등록

🖱 `frameworks/base/core/java/android/content/Context.java` — 상수 구역에 추가:

```java
public static final String DEVICE_INFO_SERVICE = "device_info";
```

🖱 `frameworks/base/core/java/android/app/SystemServiceRegistry.java` — static 블록에 추가:

```java
registerService(Context.DEVICE_INFO_SERVICE, DeviceInfoManager.class,
        new CachedServiceFetcher<DeviceInfoManager>() {
            @Override public DeviceInfoManager createService(ContextImpl ctx)
                    throws ServiceNotFoundException {
                IBinder b = ServiceManager.getServiceOrThrow(Context.DEVICE_INFO_SERVICE);
                return new DeviceInfoManager(ctx,
                        IDeviceInfoService.Stub.asInterface(b));
            }});
```

## Step 3. SystemServer 기동 코드

🖱 `frameworks/base/services/java/com/android/server/SystemServer.java` → `startOtherServices()` 내부(다른 서비스들 사이)에:

```java
t.traceBegin("StartDeviceInfoService");
try {
    ServiceManager.addService(Context.DEVICE_INFO_SERVICE,
            new com.android.server.custom.DeviceInfoService(context));
} catch (Throwable e) {
    reportWtf("starting DeviceInfoService", e);
}
t.traceEnd();
```

⚠️ 이 블록에서 **느린 작업 금지** — 부팅 전체가 늦어집니다(워크숍 S3의 씨앗).

✅ **예상 결과:** 4개 파일(Manager/Context/Registry/SystemServer) diff 준비 완료 — 옆자리와 상호 diff 리뷰

# 🏁 Pass 판정 체크리스트

- [ ] Manager: RemoteException 래핑 패턴 적용
- [ ] Context 상수 + Registry fetcher 등록 (이름 "device_info" 3곳 일치)
- [ ] SystemServer addService 블록 삽입 (try/reportWtf 포함)

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| 이름 불일치로 null 반환 예정 | "device_info" 오타 | 상수 하나(Context.DEVICE_INFO_SERVICE)만 참조하도록 통일 |
| Registry 위치 헷갈림 | static 블록 밖 삽입 | 기존 registerService 나열부 사이에 배치 |

# 🚗 현업 활용 포인트

💡 "getSystemService가 null" 이슈의 3대 용의자(장부/창구/탄생)를 오늘 전부 손으로 만들었으니, 내일부터는 **어느 파일을 열지 3초 안에** 결정할 수 있습니다.

---
*실습 D2-7 (18/36) · 다음: **D2-8 실습 6-4 SELinux + 통합 빌드 시작***
