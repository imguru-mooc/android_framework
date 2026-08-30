# [D3-0] 실습 6-5 검증 완주 세션 — 전원 Pass

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★☆☆ | Day 3 아침 · ★전원 | D2-12 진행 기록 |

> 🎯 **실습 목표** — 어제 미완/실패 지점부터 이어서 **전원 4/4 Pass**를 만든다. 실패 유형별 진단표로 스스로 원인 계층을 찾는 것이 진짜 목표.

## Step 1. 상태 자가 판정

```bat
adb shell service check device_info
```

| 결과 | 상태 | 이동할 Step |
|---|---|---|
| found | 등록 OK | Step 3 (앱/SELinux 검증) |
| not found | 등록 실패 | Step 2 |
| 부팅 자체가 안 됨 | 적용 사고 | D2-12 Step 5 복구 → 재적용 |

## Step 2. not found 진단 트리

```text
빌드 로그에 6-3 파일 포함? ─아니오→ WinSCP 저장 누락 → 재저장 후 재빌드(증분)
        │예
push한 jar가 새 빌드본? (파일 시각 비교) ─아니오→ WinSCP 재다운로드 → 재push
        │예
SystemServer 로그 확인: adb logcat -d | findstr DeviceInfoService
  → "initialized" 없음 = 기동 코드 미실행 → 6-3 Step 3 재확인
```

## Step 3. 앱·SELinux 검증

```bat
adb shell dumpsys device_info
:: 테스트앱 실행 → 값 표시
adb logcat -d | findstr /i "avc.*device_info"
```

| 증상 | 원인 계층 | 조치 |
|---|---|---|
| dumpsys OK, 앱 null | Registry(6-3②) | fetcher 등록 확인 |
| 앱 SecurityException | Java 권한 | PERM_WRITE 선언/부여 |
| 조용한 실패 + avc denied | SELinux | D2-11 5단계 (default_* 확인!) |

## Step 4. Pass 보드 기록

✅ **예상 결과:** 칠판/시트에 `stuNN: 4/4 ✅` — 전원 완료 시 Day 3 본 수업 진행

# 🏁 Pass 판정 체크리스트

- [ ] 4종 검증 4/4 달성
- [ ] (실패 경험자) 실패 원인을 계층 용어로 1문장 기록 — 예: "Registry 누락(창구 계층)"

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| 증분 빌드가 반영 안 됨 | 다른 tmux/디렉토리에서 빌드 | `tmux ls` 로 세션 확인, lunch 재실행 |
| 에뮬레이터가 어제 상태 기억 못함 | Cold Boot로 초기화됨 | .orig 백업부터 D2-12 Step 2 재수행 |

# 🚗 현업 활용 포인트

💡 오늘 만든 **진단 트리 2장**이 이 과정의 첫 '자작 런북'입니다. 실무 위키에 같은 형식으로 축적하는 팀이 장애 대응이 빠른 팀입니다.

---
*실습 D3-0 (24/36) · 다음: **D3-1 관찰 3-A AMS·프로세스***
