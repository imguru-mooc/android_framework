# [D2-3] 실습 4-3 — TransactionTooLargeException 재현과 대안표

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★☆☆☆ | Day 2 Ch 4 · 필수 | 로컬 에뮬레이터 · 아무 테스트 앱 프로젝트 · 🎬 ⑯ 선시청 |

> 🎯 **실습 목표** — 1.5MB 데이터를 Intent에 실어 **의도적으로 예외를 재현**하고, 커널 버퍼 현황을 확인한 뒤 대용량 대안표를 완성한다.

## Step 1. 재현 코드 (버튼 하나짜리 액티비티)

```kotlin
button.setOnClickListener {
    val big = ByteArray(1_500_000)          // 1.5MB
    val i = Intent(this, SecondActivity::class.java)
    i.putExtra("big", big)
    startActivity(i)                        // Intent도 Binder를 탄다!
}
```

## Step 2. 실행 → 예외 관찰

```bat
adb logcat -s JavaBinder:E AndroidRuntime:E | findstr /i "FAILED TransactionTooLarge"
```

✅ **예상 결과:**

```text
E JavaBinder: !!! FAILED BINDER TRANSACTION !!!  (parcel size = 1572864)
android.os.TransactionTooLargeException: data parcel size 1572864 bytes
```

## Step 3. 임계점 찾기 + 커널 버퍼 확인

크기를 바꿔가며(1.5MB → 800KB → 500KB → 200KB) 성공/실패 경계를 찾고:

```bat
adb shell "cat /sys/kernel/debug/binder/proc/$(pidof <내앱패키지의PID>) | grep -A5 allocated"
```

✅ **예상 결과:** 대략 수백 KB 근처에서 성공 시작(그 순간의 **동시 트랜잭션 총량**에 따라 흔들림 — 그것이 ⑯의 '공유 물탱크' 교훈)

## Step 4. 대안표 완성

| 방법 | 원리 | 적합 상황 |
|---|---|---|
| **ashmem/SharedMemory** | 공유메모리 + FD 전달 | 프로세스 간 대용량 버퍼 |
| **FD(ParcelFileDescriptor)** | 열쇠만 Binder 통과 | 파일/파이프 스트리밍 |
| **ContentProvider/URI** | 수신측이 열어 읽음 | 앱 간 파일·이미지 |
| **MemoryFile** | ashmem의 Java 래퍼 | 임시 대용량 캐시 |

# 🏁 Pass 판정 체크리스트

- [ ] FAILED BINDER TRANSACTION 로그 캡처
- [ ] 성공/실패 경계 크기 실측 기록(±값)
- [ ] 대안표 4행 완성 + "Intent extras도 같은 버퍼"를 1문장으로 설명

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| 예외가 안 나고 그냥 됨 | 같은 프로세스 내 Activity | 대상 Activity에 `android:process=":second"` 지정 |
| 앱이 예외 없이 사라짐 | 예외가 시스템측에서 발생 | logcat 전체(-d)에서 JavaBinder 라인 검색 |

# 🚗 현업 활용 포인트

💡 "화면 회전만 하면 죽어요"의 단골 원인이 onSaveInstanceState에 실린 Bitmap — 오늘 예외의 쌍둥이입니다. 크래시 리포트에서 `parcel size` 숫자를 보는 순간 이 실습을 떠올리세요.

---
*실습 D2-3 (14/36) · 다음: **D2-4 실습 5-1 호출 스택 추적***
