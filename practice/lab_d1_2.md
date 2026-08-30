# [D1-2] 관찰 1-B — Binder의 흔적: CursorWindow와 커널 state

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★☆☆☆ | Day 1 Ch 1 직후 · 필수 | Automotive Emulator (adb root) |

> 🎯 **실습 목표** — 아직 코드를 짜기 전에, 시스템이 이미 쓰고 있는 Binder의 흔적 두 곳 — ContentProvider의 **CursorWindow**(공유메모리)와 커널 **binder/state** — 을 들여다본다.

## Step 1. root + debugfs 준비

```bat
adb root
adb shell ls /sys/kernel/debug/binder/
```

⚠️ 목록이 비거나 오류면 debugfs 미마운트:

```bat
adb shell mount -t debugfs none /sys/kernel/debug
```

✅ **예상 결과:** `failed_transaction_log  state  stats  transaction_log  transactions`

## Step 2. binder/state 헤드 읽기

```bat
adb shell "cat /sys/kernel/debug/binder/state | head -30"
```

✅ **예상 결과:** `binder state:` 아래로 proc 별 스레드/노드/ref 통계 — 지금은 **"이런 장부가 있다"**만 확인. Day 2(4-1/4-3)에서 본격 해석합니다.

## Step 3. CursorWindow 흔적 — Provider의 공유메모리

```bat
adb shell dumpsys activity provider | findstr /i "cursorwindow published"
adb shell dumpsys meminfo com.android.car | findstr /i "ashmem"
```

✅ **예상 결과:** Provider 목록(published=true)과 프로세스의 ashmem 사용 라인 — "큰 데이터는 Parcel이 아니라 공유메모리로"(애니메이션 ⑭)의 실증입니다.

# 🏁 Pass 판정 체크리스트

- [ ] binder 디버그 5파일 확인 (필요 시 debugfs 마운트)
- [ ] state 헤드 30줄 확보
- [ ] provider/ashmem 라인 확인

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| Permission denied | adb root 누락 | `adb root` 후 재시도 (Google APIs/Automotive 이미지 필수) |
| binder 디렉토리 자체가 없음 | debugfs 미마운트 | Step 1의 mount 명령 |

# 🚗 현업 활용 포인트

💡 `binder/state`는 실무에서 "누가 Binder 버퍼를 먹고 있나"를 보는 창입니다. 오늘 위치만 알아두면, Day 2의 TransactionTooLarge 분석(4-3)과 워크숍에서 자연스럽게 손이 갑니다.

---
*실습 D1-2 (9/36) · 다음: **D1-3 Lab 1-1 CalcServiceApp***
