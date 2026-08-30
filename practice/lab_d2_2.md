# [D2-2] 실습 4-2 — Binder Thread Pool 관찰과 데드락 조건

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★☆☆ | Day 2 Ch 4 · 필수 | 로컬 에뮬레이터 (adb root) · 🎬 ⑮ 선시청 |

> 🎯 **실습 목표** — system_server의 Binder 스레드 풀을 실측하고, **상호 동기 호출 + 풀 고갈**이라는 데드락 성립 조건을 도식으로 완성한 뒤 해결 4안을 표로 정리한다.

## Step 1. Binder 스레드 세어 보기

```bat
adb shell "ps -T -p $(pidof system_server) | grep -c binder:"
adb shell "ps -T -p $(pidof system_server) | grep binder: | head -5"
```

✅ **예상 결과:** `binder:PID_N` 이름의 스레드 다수(십수 개 안팎) — 요청이 올 때마다 이 풀에서 하나가 배정됩니다.

## Step 2. 커널 시점의 스레드/대기 상태

```bat
adb shell "cat /sys/kernel/debug/binder/proc/$(pidof system_server) | grep -E 'thread|requested' | head -10"
```

✅ **예상 결과:** `threads: N`, `requested threads: ...(max 15)` 류 — **최대치가 유한**하다는 것이 데드락의 전제조건입니다.

## Step 3. 데드락 조건 도식 완성 (지면 실습)

아래 빈칸을 채워 도식을 완성하세요.

```text
프로세스 A ──(동기 transact)──▶ 프로세스 B
    ▲                              │
    └────(동기 transact)───────────┘
성립 조건: ① 양방향 모두 [    동기    ] 호출
          ② A의 가용 Binder 스레드가 [   0개(고갈)   ]
결과: B→A 호출이 영원히 대기 → 상호 대기 고리 💀
```

💡 검증 아이디어(선택): Day 1의 CalcService 2개를 서로 `:remote`로 바인딩해 **상호 동기 호출**을 걸고, traces에서 `transactNative` 대기 스레드를 관찰.

## Step 4. 해결 4안 표 완성

| # | 해법 | 핵심 | 대가/주의 |
|---|---|---|---|
| 1 | **oneway 전환** | 역방향 통지를 비동기로 → 고리 절단 | 리턴/예외 전달 불가 |
| 2 | 호출 방향 단일화 | A→B만 허용, 역방향은 콜백 큐 | 설계 변경 필요 |
| 3 | 스레드 풀 확대 | `setThreadPoolMaxThreadCount` | 근본 해결 아님(지연만) |
| 4 | 아키텍처 재설계 | 공유 상태/브로커 도입 | 비용 최대, 효과 최대 |

# 🏁 Pass 판정 체크리스트

- [ ] system_server binder 스레드 수 실측 기록
- [ ] 도식의 빈칸 2개를 정확한 용어로 완성
- [ ] 해결 4안 표 완성(대가 칸 포함)

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| grep -c 가 0 | 스레드 이름 패턴 상이 | `ps -T` 출력에서 실제 이름(binder:…) 확인 후 패턴 조정 |
| proc 파일 없음 | root/debugfs | D2-1 Step 준수 |

# 🚗 현업 활용 포인트

💡 실무 데드락의 신호는 "**binder 스레드 전원이 transactNative에서 잠듦**"(traces) 입니다. 오늘의 도식이 그 traces를 3초 만에 읽게 해 주는 안경입니다 — ⑳ 3단 독법과 한 세트.

---
*실습 D2-2 (13/36) · 다음: **D2-3 실습 4-3 TransactionTooLarge 재현***
