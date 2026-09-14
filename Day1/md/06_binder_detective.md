# 실습 6. Binder 탐정 — System Service 추적하기

> **소요시간:** 20분 · **난이도:** ★☆☆ · **챕터:** Ch 1 Binder

## 목표

- App이 System Service를 호출할 때 Binder Transaction이 발생하는 것을 실시간 관찰한다.
- `service call` 로 System Service를 직접 호출한다.

---

## Step 1. `service call` 로 직접 Service 호출

```bash
adb root
adb shell
```

```bash
# clipboard Service 호출 (IClipboard transaction code 1)
service call clipboard 1

dumpsys display | grep "mBaseDisplayInfo"
dumpsys battery
```

`service call <name> <code>` 는 AIDL Method 순서(1부터)를 그대로 Transaction code로 사용한다. Proxy 없이도 Binder Driver를 통해 호출된다는 것을 보여 준다.

## Step 2. Binder Transaction 실시간 관찰 (ftrace)

```bash
# 1. 이벤트 활성화 확인 (1 이어야 함)
cat /sys/kernel/tracing/events/binder/binder_transaction/enable
echo 1 > /sys/kernel/tracing/events/binder/binder_transaction/enable

# 2. tracing_on 확인
cat /sys/kernel/tracing/tracing_on
echo 1 > /sys/kernel/tracing/tracing_on

# 3. 실시간 출력
cat /sys/kernel/tracing/trace_pipe
```

Emulator에서 앱을 열거나 설정을 바꾸면 Transaction 로그가 흘러나온다.

```text
 com.android.car-1832  [001] ...  binder_transaction: transaction=12345 dest_node=... dest_proc=612 dest_thread=0 reply=0 flags=0x10 code=0x2
```

| 필드 | 의미 |
|---|---|
| `dest_proc` | 수신 Process PID (system_server 등) |
| `reply` | 0 = 요청, 1 = 응답 |
| `flags & 0x1` | oneway |
| `code` | Transaction code |

```bash
# 멈추기 / 비활성화
echo 0 > /sys/kernel/tracing/tracing_on
echo 0 > /sys/kernel/tracing/events/binder/binder_transaction/enable
```

## Step 3. 특정 App의 Binder 사용량 추적

```bash
SETPID=$(pidof com.android.car.settings)

cat /dev/binderfs/binder_logs/proc/$SETPID

# Binder Thread 수
cat /dev/binderfs/binder_logs/proc/$SETPID | grep "thread"
```

---

## 🎯 결과물

- Settings 앱을 열 때 발생하는 Binder Transaction 수
- `dest_proc` 을 PID로 역추적해 어떤 Service가 호출되는지 추정

## 핵심 정리

- App ↔ system_server 통신은 예외 없이 Binder Driver를 거친다.
- `binderfs/binder_logs` 와 ftrace `binder_transaction` 은 Binder 병목 분석의 기본 도구다.
