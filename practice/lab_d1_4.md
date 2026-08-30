# [D1-4] Lab 1-2 — oneway 콜백 서비스 + 대용량 URI 우회

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★☆☆ | Day 1 Ch 3 · 필수 | 배포 프로젝트(콜백 완성본) |

> 🎯 **실습 목표** — 서버→클라 역방향 통신(**oneway 콜백 + RemoteCallbackList**)을 완성하고, 1MB를 넘는 데이터가 왜 실패하는지 재현한 뒤 **URI 전달**로 우회한다.

## Step 1. 콜백 계약 확인

```java
// ICallback.aidl
oneway interface ICallback {
    void onResult(int value);
}
// ICalcService.aidl (발췌)
void registerCallback(ICallback cb);
void unregisterCallback(ICallback cb);
void computeAsync(int a, int b);   // 결과는 콜백으로
```

❓ **왜 oneway?** 서버가 느린 앱을 기다리다 스레드를 뺏기지 않기 위해 — 애니메이션 ③⑮의 그 이유.

## Step 2. 서버 — RemoteCallbackList로 보관·브로드캐스트

```kotlin
private val callbacks = RemoteCallbackList<ICallback>()

override fun registerCallback(cb: ICallback) { callbacks.register(cb) }
override fun unregisterCallback(cb: ICallback) { callbacks.unregister(cb) }

override fun computeAsync(a: Int, b: Int) {
    handlerThread.post {                    // 무거운 일은 워커에서
        val r = a + b
        val n = callbacks.beginBroadcast()
        for (i in 0 until n) {
            try { callbacks.getBroadcastItem(i).onResult(r) }
            catch (e: RemoteException) { /* oneway: 상대가 죽었을 수 있음 */ }
        }
        callbacks.finishBroadcast()
    }
}
```

## Step 3. 클라 — 수신은 Binder 스레드, UI는 메인으로

```kotlin
private val cb = object : ICallback.Stub() {
    override fun onResult(value: Int) {
        runOnUiThread { result.text = "callback: $value" }
    }
}
// onServiceConnected: svc.registerCallback(cb); svc.computeAsync(40, 2)
// onDestroy: svc?.unregisterCallback(cb)
```

✅ **예상 결과:** 버튼 → 잠시 후 `callback: 42`. **runOnUiThread를 지우면** `CalledFromWrongThreadException` — 지웠다 복구해 보세요(콜백=Binder 스레드의 증거).

## Step 4. 1MB 초과 재현 → URI 우회

```kotlin
// (a) 실패 재현: byte[] 1.5MB를 AIDL로 직접 전달
svc.sendBigData(ByteArray(1_500_000))   // → TransactionTooLargeException 로그 확인
```

```kotlin
// (b) 우회: 파일로 쓰고 URI만 전달
val f = File(cacheDir, "big.bin").apply { writeBytes(ByteArray(1_500_000)) }
val uri = FileProvider.getUriForFile(this, "$packageName.fp", f)
grantUriPermission(serverPkg, uri, Intent.FLAG_GRANT_READ_URI_PERMISSION)
svc.sendBigDataUri(uri.toString())      // 서버가 contentResolver로 열어 읽기
```

✅ **예상 결과:** (a) `!!! FAILED BINDER TRANSACTION !!!` / (b) 서버 로그에 `read 1500000 bytes` — **내용 대신 열쇠**(애니메이션 ⑭)

# 🏁 Pass 판정 체크리스트

- [ ] 콜백으로 42 수신 (등록→브로드캐스트→해제 흐름 구현)
- [ ] runOnUiThread 제거 시 크래시 확인 후 복구
- [ ] 1.5MB 직접 전달 실패 로그 + URI 우회 성공 로그

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| 콜백이 한 번도 안 옴 | register 전에 computeAsync 호출 | onServiceConnected 안에서 순서 보장 |
| unregister 안 해서 액티비티 재생성 후 중복 수신 | 해제 누락 | onDestroy에서 unregisterCallback |
| SecurityException(URI) | grantUriPermission 누락 | Step 4(b)의 grant 라인 확인 |

# 🚗 현업 활용 포인트

💡 CarPropertyManager.registerCallback이 정확히 이 패턴(콜백 등록 + 시스템→앱 oneway)입니다. "등록과 해제는 쌍, 큰 데이터는 열쇠로" — 차량 앱 리뷰 체크리스트의 고정 항목.

---
*실습 D1-4 (11/36) · 다음: **D2-1 실습 4-1 mmap 관찰***
