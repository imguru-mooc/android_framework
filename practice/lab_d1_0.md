# [D1-0] 환경 삼중 점검 + tmux 3명령 리허설

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★☆☆☆☆ | Day 1 오전 오리엔테이션 · 전원 | Day 0 완료 상태 |

> 🎯 **실습 목표** — ① 로컬 Automotive Emulator ② PuTTY+tmux+WinSCP ③ AOSP 빌드 상태, 3가지를 5분 안에 점검하는 자기 진단 루틴을 만들고, tmux 3명령을 전원 1회 손으로 반복한다. 미통과 항목은 휴식시간 개별 지원 대상.

## Step 1. ① 로컬 에뮬레이터 점검

```bat
adb devices
adb shell getprop ro.build.version.sdk
adb shell pidof com.android.car
```

✅ **예상 결과:** device 목록 1개 이상 / `36` / CarService PID

## Step 2. ② 원격 3종 점검 (PuTTY·tmux·WinSCP)

```bash
# PuTTY 접속 후
tmux new -s day1   # 새 세션
# Ctrl+b, d 로 분리
tmux ls            # day1 세션 확인
tmux attach -t day1
exit               # 세션 종료
```

🖱 WinSCP 접속 → 북마크 4종(edu/custom/OUT/apex) 이동 확인

✅ **예상 결과:** new→detach→ls→attach 왕복 성공, 북마크 4종 모두 이동됨

## Step 3. ③ AOSP 빌드 상태 점검

```bash
ls -lh ~/aosp/out/target/product/emulator_car64_x86_64/system.img
tail -3 ~/build_full.log
```

✅ **예상 결과:** system.img 존재 + `build completed successfully`

# 🏁 Pass 판정 체크리스트

- [ ] ① 에뮬레이터: SDK 36 + com.android.car PID 확인
- [ ] ② tmux new/detach/attach 왕복 + WinSCP 북마크 4종
- [ ] ③ $OUT/system.img 존재

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| adb devices 비어 있음 | 에뮬레이터 미부팅/adb 꼬임 | 에뮬레이터 부팅 후 `adb kill-server && adb start-server` |
| tmux ls: no server running | 세션을 만든 적 없음 | `tmux new -s day1` 부터 |
| system.img 없음 | D0-2 미완료 | 즉시 tmux에서 빌드 시작 — Day 2 오후 전 완료 필수 |

# 🚗 현업 활용 포인트

💡 장비 이슈의 절반은 "환경이 어제와 다름"에서 옵니다. 오늘 만든 3줄 점검 루틴을 **매일 아침 습관**으로 — 문제를 실습 중이 아니라 실습 전에 발견하는 것이 프로의 시간 관리입니다.

---
*실습 D1-0 (7/36) · 다음: **D1-1 관찰 1-A 시스템 지도***
