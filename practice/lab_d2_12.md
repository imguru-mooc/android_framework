# [D2-12] 실습 6-5 — 산출물 전송·적용·검증 착수

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★★☆ | Day 2 마감(빌드 완료분) · ★전원 필수 | WinSCP(OUT 북마크) + 로컬 에뮬레이터 · 🎬 ㉙⑤ 선시청 |

> 🎯 **실습 목표** — 서버 빌드 산출물(framework.jar/services.jar)을 로컬 에뮬레이터에 적용하고 4종 검증에 착수한다. **백업 → 적용 → 검증 → (실패 시) 복구**의 전 사이클.

## Step 1. 빌드 완료 확인 + 산출물 다운로드 (WinSCP)

```bash
tmux attach -t build     # "#### build completed successfully ####" 확인 후 detach
```

🖱 WinSCP 북마크 **OUT** → `system/framework/` 이동 → `framework.jar`, `services.jar` 를 로컬 작업 폴더(예: `C:\aosp_out\`)로 드래그

## Step 2. 로컬 에뮬레이터 준비 + 원본 백업

```bat
cd %LOCALAPPDATA%\Android\Sdk\emulator
emulator -avd Automotive_API36 -writable-system
:: (새 터미널)
adb root & adb remount
adb shell cp /system/framework/framework.jar /system/framework/framework.jar.orig
adb shell cp /system/framework/services.jar  /system/framework/services.jar.orig
```

⚠️ **백업 없이 다음 단계 금지** — 부팅 루프의 유일한 보험입니다.

## Step 3. push + 재부팅

```bat
adb push C:\aosp_out\framework.jar /system/framework/
adb push C:\aosp_out\services.jar  /system/framework/
adb reboot
adb wait-for-device
```

## Step 4. 4종 검증

```bat
adb shell service check device_info
adb shell dumpsys device_info
:: 테스트앱 설치·실행 → 모델명/uptime 값 표시 확인
adb logcat -d | findstr /i "avc denied DeviceInfo"
```

✅ **예상 결과:** ① `Service device_info: found` ② dumpsys 정보 출력 ③ 테스트앱 값 표시 ④ avc denied **없음** — 4/4면 즉시 Pass 보드 기록!

## Step 5. 🚑 실패 시 복구 절차

```bat
:: 부팅 루프/블랙스크린:
adb wait-for-device shell mv /system/framework/framework.jar.orig /system/framework/framework.jar
adb shell mv /system/framework/services.jar.orig /system/framework/services.jar
adb reboot
:: adb 자체가 안 붙으면: Device Manager → Cold Boot Now → 위 복원 재시도
```

💡 미완료자는 **D3-0(내일 아침 검증 세션)** 에서 완주 — 오늘은 어디까지 갔는지 단계 번호만 기록해 두세요.

# 🏁 Pass 판정 체크리스트

- [ ] .orig 백업 2종 존재 확인 후 push
- [ ] 4종 검증 결과 기록 (4/4 또는 실패 단계 번호)
- [ ] 복구 절차를 보지 않고 말로 재현 가능

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| remount 실패 | -writable-system 미부팅 | Step 2 부팅 옵션 재확인(D0-1 Step 6) |
| service not found | SystemServer 등록 코드 미포함 빌드 | 6-3 diff 확인 후 재빌드 |
| found인데 앱에서 null | Registry 미등록 | 6-3 Step 2 확인 |
| found인데 접근 거부/조용한 실패 | SELinux | logcat avc → D2-11 절차 |

# 🚗 현업 활용 포인트

💡 오늘의 사이클(백업→적용→검증→복구)이 곧 **플랫폼 패치 검증의 표준 운영 절차(SOP)** 입니다. 실패 단계 번호로 대화하는 습관("3단계에서 service not found")이 팀 디버깅 속도를 바꿉니다.

---
*실습 D2-12 (23/36) · 다음: **D3-0 6-5 검증 완주 세션***
