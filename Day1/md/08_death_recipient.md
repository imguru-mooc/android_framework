# 실습 8. Binder 생존 게임 — DeathRecipient & 복구

> **소요시간:** 30분 · **난이도:** ★★☆ · **챕터:** Ch 2 AIDL
> **환경:** 실습 7 `BinderLab` 프로젝트에서 계속 진행

## 목표

- Service Process가 죽었을 때의 동작을 체험한다.
- `DeathRecipient` 콜백으로 자동 복구 패턴을 구현한다.

---

## Step 1. DeathRecipient 추가

**`MainActivity.java`** 에 다음을 추가한다.

```java
// 필드 추가
private IBinder serviceBinder;
private int deathCount = 0;

// ★ Service Process 사망 감지 — Binder Thread 에서 호출된다
private final IBinder.DeathRecipient deathRecipient = new IBinder.DeathRecipient() {
    @Override
    public void binderDied() {
        deathCount++;
        Log.e(TAG, "☠ Service process died! (count=" + deathCount + ") thread="
                + Thread.currentThread().getName());
        bound = false;
        service = null;

        // ★ UI 갱신은 반드시 Main Thread 로
        runOnUiThread(() -> {
            infoText.setText("☠ Service DIED! Rebinding... (#" + deathCount + ")");
            resultText.setText("Service 가 죽었습니다. 자동 재바인딩 중...");
        });

        // 기존 연결 해제 후 재바인딩
        try { unbindService(connection); } catch (Exception ignored) {}
        Intent intent = new Intent("com.example.binderlab.CALCULATOR");
        intent.setPackage(getPackageName());
        bindService(intent, connection, Context.BIND_AUTO_CREATE);
    }
};
```

**`onServiceConnected`** 를 수정한다.

```java
@Override
public void onServiceConnected(ComponentName name, IBinder binder) {
    serviceBinder = binder;
    service = ICalculatorService.Stub.asInterface(binder);
    bound = true;
    try {
        // ★ 사망 감지 등록
        binder.linkToDeath(deathRecipient, 0);
        infoText.setText("✅ 연결됨 (사망 횟수 " + deathCount + ")\n" + service.getCallerInfo());
    } catch (RemoteException e) {
        infoText.setText("linkToDeath 실패: " + e);
    }
}

@Override
public void onServiceDisconnected(ComponentName name) {
    Log.w(TAG, "onServiceDisconnected — thread=" + Thread.currentThread().getName());
    bound = false;
    service = null;
}
```

**`onDestroy`** 에서 해제:

```java
@Override
protected void onDestroy() {
    if (serviceBinder != null) serviceBinder.unlinkToDeath(deathRecipient, 0);
    if (bound) { unbindService(connection); bound = false; }
    super.onDestroy();
}
```

## Step 2. Service Process 강제 종료

앱을 실행한 뒤:

```bash
adb root
adb shell "ps -A | grep calc_remote"
adb shell "kill -9 $(adb shell pidof com.example.binderlab:calc_remote)"
```

## Step 3. 관찰할 것

1. 앱 화면에 **"☠ Service DIED! Rebinding..."** 표시
2. Logcat 에 `☠ Service process died!` 로그와 **호출 Thread 이름** (`Binder:xxxx_1`)
3. 자동 재바인딩 후 계산이 다시 동작
4. `getCallerInfo()` 의 **Service PID 가 바뀜** (새 Process)
5. Logcat 순서: `binderDied` → `onServiceDisconnected` → `onServiceConnected`

```bash
adb logcat -s CalcClient CalcService
```

## Step 4. 반복 Kill 스트레스 테스트

```bash
adb shell
```

```bash
for i in 1 2 3 4 5; do
  PID=$(pidof com.example.binderlab:calc_remote)
  if [ -n "$PID" ]; then
    echo "Killing PID $PID (attempt $i)"
    kill -9 $PID
  fi
  sleep 3
done
```

앱이 매번 자동 복구되고 사망 횟수가 5까지 올라가는지 확인한다.

---

## 🎯 핵심 학습 포인트

| 관찰 | 설명 |
|---|---|
| `binderDied()` 가 Binder Thread 에서 호출 | UI 접근은 `runOnUiThread()` 필수 |
| `binderDied()` 가 `onServiceDisconnected()` 보다 먼저 | Driver 가 직접 감지 vs AMS 경유 (Main Thread) |
| `BIND_AUTO_CREATE` | Service Process 가 죽으면 시스템이 자동 재생성 |
| Service PID 변경 | 새 Process 이므로 Service 내부 상태(history)는 초기화됨 |

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `binderDied` 가 호출되지 않음 | `linkToDeath` 를 `onServiceConnected` 안에서 등록했는지 확인 |
| 재바인딩 후 `DeadObjectException` | 이전 `service` Proxy 를 계속 쓰고 있음 → `onServiceConnected` 에서 새 Proxy 로 교체 |
| `pidof` 결과 없음 | Service 가 아직 뜨지 않았거나 이미 죽은 상태 → 앱에서 계산 한 번 실행 |
