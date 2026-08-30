# [D2-1] 실습 4-1 — Binder mmap Zero-Copy 관찰

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★☆☆☆ | Day 2 Ch 4 · 필수 | 로컬 Automotive Emulator (adb root) · 🎬 ④ 선시청 |

> 🎯 **실습 목표** — system_server의 `/dev/binder` mmap 영역(≈1MB, **r--p**)을 직접 읽고, 여러 프로세스와 비교하며 커널 `binder/state`까지 확인해 **"1회 복사 + 읽기 전용 공유"** 를 실측한다.

## Step 1. system_server의 mmap 영역 확인

```bat
adb root
adb shell pidof system_server
adb shell "cat /proc/$(pidof system_server)/maps | grep -E '/dev/(binder|hwbinder|vndbinder)'"
```

✅ **예상 결과:**

```text
7f8a00000000-7f8a000ff000 r--p 00000000 00:06 1234  /dev/binder
```

기록표 채우기: 시작 주소 ____ / 크기(끝-시작) ____ / 권한 ____

❓ **해석** — `0xff000` ≈ **1MB**(트랜잭션 한도와 일치), `r--p` = 서버는 **읽기만**(넣는 쪽은 커널) — 애니메이션 ④의 두 핵심 숫자·권한이 그대로 보입니다.

## Step 2. 프로세스 3종 비교

```bat
adb shell "for p in system_server surfaceflinger audioserver; do echo === $p ===; cat /proc/$(pidof $p)/maps | grep /dev/binder; done"
```

✅ **예상 결과:** 셋 다 각자의 주소에 ≈1MB r--p 매핑 — **프로세스마다 자기 창**을 가진다는 증거(주소는 서로 다름!)

## Step 3. 커널 장부 — binder/state

```bat
adb shell "cat /sys/kernel/debug/binder/state | head -30"
adb shell "cat /sys/kernel/debug/binder/proc/$(pidof system_server) | grep -A5 allocated"
```

✅ **예상 결과:** proc 통계 + `buffers allocated` 류의 현재 할당 수 — 기록표 마지막 칸(버퍼 수) 채우기

# 🏁 Pass 판정 체크리스트

- [ ] 기록표 4칸 완성 (주소/크기/권한/버퍼 수)
- [ ] 크기≈1MB, 권한 r--p 를 자신의 말로 1문장 설명
- [ ] 3개 프로세스의 매핑 주소가 서로 다름을 확인

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| maps에 binder 라인 없음 | grep 패턴/따옴표 문제 | 위 명령 그대로 복사 (셸 인용 주의) |
| debug/binder 접근 불가 | debugfs 미마운트 | `adb shell mount -t debugfs none /sys/kernel/debug` |
| pidof 빈 값 | 부팅 직후 | 부팅 완료(boot_completed=1) 후 재시도 |

# 🚗 현업 활용 포인트

💡 "Binder가 느리다/터진다"는 이슈에서 첫 팩트 수집이 이 세 명령입니다. maps로 창 크기, state로 사용량 — **감이 아니라 숫자**로 말하는 훈련.

---
*실습 D2-1 (12/36) · 다음: **D2-2 실습 4-2 Thread Pool & 데드락***
