# [D2-10] 실습 5-2 — Binder 보안 검증: 커널 주입 UID/PID

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★☆☆☆ | Day 2 오후 2교시 · 필수 | 로컬 에뮬레이터 (adb root) |

> 🎯 **실습 목표** — `service call`로 임의 트랜잭션을 쏘고, 커널 `transaction_log`에서 **호출자 PID/UID가 커널에 의해 기록**됨을 확인해 "userspace 위변조 불가"를 실증한다.

## Step 1. 트랜잭션 발사

```bat
adb root
adb shell service call activity 1
```

✅ **예상 결과:** `Result: Parcel(...)` — 내용보다 "쐈다"는 사실이 중요

## Step 2. 커널 장부에서 방금 호출 찾기

```bat
adb shell "cat /sys/kernel/debug/binder/transaction_log | tail -10"
```

✅ **예상 결과(형식):**

```text
NNNN: reply from 1842:1900 to 4021:4021 ... 
NNNN: call  from 4021:4021 to 1842:0 node ... size 8:0 ...
```

`from <PID>:<TID>` 가 **shell 프로세스의 실제 PID** — 우리가 Parcel에 쓴 적 없는 값입니다.

## Step 3. 교차 검증

```bat
adb shell "ps -A | grep -E ' sh$|shell'"
```

✅ **예상 결과:** transaction_log의 from PID = shell(또는 service 명령 프로세스)의 PID — **커널이 붙인 명찰**(🎬 ②⑬)

# 🏁 Pass 판정 체크리스트

- [ ] transaction_log에서 내 호출 라인 식별 (from PID 기록)
- [ ] from PID = 실제 프로세스 PID 교차 확인
- [ ] "getCallingUid를 신뢰할 수 있는 이유"를 한 문장으로

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| transaction_log 접근 불가 | root/debugfs | `adb root` + debugfs mount(D1-2) |
| 내 라인을 못 찾음 | 로그 밀림 | 호출 직후 즉시 tail, 또는 `grep "call "` 로 축약 |

# 🚗 현업 활용 포인트

💡 보안 리뷰에서 "클라이언트가 UID를 속이면요?"라는 질문에, 오늘의 로그 한 장이 최종 답변입니다. 권한 모델의 신뢰 사슬은 커널에서 시작합니다.

---
*실습 D2-10 (21/36) · 다음: **D2-11 SELinux avc 디버깅***
