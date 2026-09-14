# 실습 5. 시스템 탐험가 — adb로 Android 내부 들여다보기

> **소요시간:** 20분 · **난이도:** ★☆☆ · **챕터:** Ch 0 Architecture & Boot

## 목표

- Android의 실제 Process 구조를 눈으로 확인한다.
- Zygote, system_server, ServiceManager의 존재를 직접 확인한다.
- System Service 목록과 Binder 상태를 실시간으로 본다.

---

## Step 1. Process 구조 확인

```bash
adb root
adb shell
```

```bash
# 전체 Process 트리 — Zygote가 App의 부모임을 확인
ps -A -o pid,ppid,name | head -30

# Zygote 찾기
ps -A | grep zygote
# root  1234  1  zygote64
# root  1235  1  zygote

# system_server 찾기 (Zygote의 자식)
ps -A | grep system_server
# PPID 가 zygote64 의 PID 와 같은지 확인

# init (PID 1)
ps -A -o PID,PPID,NAME | awk '$1 == 1'
```

**✅ 확인 포인트**

- Zygote의 PPID = 1 (init)
- system_server의 PPID = Zygote PID
- 다른 App Process의 PPID도 Zygote → **모든 App은 Zygote에서 fork**

## Step 2. System Service 탐색

```bash
# 등록된 System Service 목록 (100개+)
service list
# 0 sip: [android.net.sip.ISipService]
# 1 phone: [com.android.internal.telephony.ITelephony]
# 2 activity: [android.app.IActivityManager]

service list | wc -l

dumpsys activity | head -50
dumpsys window displays | head -30
dumpsys package com.android.car.settings | head -30
```

## Step 3. Binder 상태 확인

```bash
cat /dev/binderfs/binder_logs/stats | head -20

SYSPID=$(pidof system_server)
cat /dev/binderfs/binder_logs/proc/$SYSPID | head -30

cat /dev/binderfs/binder_logs/stats | grep "BC_TRANSACTION"
```

## Step 4. Boot Sequence 확인

```bash
dmesg | grep -i "binder"
logcat -b events -d | grep "boot"

# Zygote Service 정의
cat /system/etc/init/hw/init.zygote64_32.rc
```

`init.zygote64_32.rc` 에서 다음을 찾아 본다.

```text
service zygote /system/bin/app_process64 -Xzygote /system/bin --zygote --start-system-server --socket-name=zygote
    class main
    priority -20
    user root
    group root readproc reserved_disk
    socket zygote stream 660 root system
```

`--start-system-server` 옵션 때문에 Zygote가 system_server를 fork한다.

---

## 🎯 결과물 (보고서)

1. Zygote PID와 system_server PID
2. System Service 총 개수
3. 가장 많은 Binder Transaction을 처리하는 Service 3개 추정

## 핵심 정리

| 구성 요소 | 역할 |
|---|---|
| init (PID 1) | init.rc 해석, Service 실행 |
| Zygote | Framework preload 후 fork로 App Process 생성 |
| system_server | AMS, WMS, PMS 등 System Service 실행 |
| ServiceManager | Binder Service 등록·조회 |
