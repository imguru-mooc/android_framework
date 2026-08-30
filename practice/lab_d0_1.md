# [D0-1] Windows 11 로컬 실습 환경 구축 — Android Studio · Automotive Emulator

<!-- ═══════════════ 강사 편집 가이드 ═══════════════
이 md만 수정하면 lab.html?file=labs_md/lab_d0_1.md 가 자동 반영합니다.
· 제목: "# [실습ID] 제목"
· 스텝: "## Step 1. 제목" → 번호·체크박스 카드로 자동 변환
· 문단 첫 이모지로 색 박스: ✅결과(초록) ⚠️주의(노랑) 💡팁(파랑) 🖱GUI경로(회색) ❓왜?(회색)
· Pass 체크리스트: "- [ ] 항목"
· 코드는 ``` 블록 — 복사 버튼 자동
════════════════════════════════════════════════ -->

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★☆☆☆☆ (입문) | Day 0 — 교육 1주 전까지 ★전원 필수 | Windows 11 · 관리자 권한 · 디스크 30GB↑ |

> 🎯 **실습 목표** — ① Android Studio + SDK 36(Android 16) 설치, ② **adb root가 가능한 Google APIs 이미지**와 **Automotive Emulator** 두 AVD 생성, ③ Day 2~4 시스템 수정 실습 대비 **`-writable-system` 부팅 + adb root/remount** 성공.
> 이 환경은 4일 내내 "관찰(adb/dumpsys)과 검증(push 후 확인)"의 무대입니다. 빌드는 서버(D0-2), 실행·확인은 전부 이 로컬 에뮬레이터에서 합니다.

## Step 1. 가상화 기능 확인 — Hyper-V · WHPX 켜기

❓ **왜?** x86_64 에뮬레이터는 하드웨어 가상화 위에서 돕니다. Windows 11에서는 **Hyper-V + Windows 하이퍼바이저 플랫폼(WHPX)** 조합이 표준입니다.

🖱 **Windows 검색 → "Windows 기능 켜기/끄기"** → 아래 두 항목 체크 → 확인 → **재부팅**
▸ **Hyper-V** (전체) ▸ **Windows 하이퍼바이저 플랫폼**

재부팅 후 PowerShell(관리자)에서 상태를 확인합니다.

```powershell
systeminfo | findstr /i "Hyper-V"
# 또는
Get-ComputerInfo -Property "HyperV*"
```

✅ **예상 결과:** `Hyper-V 요구 사항: 하이퍼바이저가 검색되었습니다...` 류의 문구, 또는 HyperV 관련 값들이 `True`

⚠️ **Home 에디션**은 Hyper-V 항목이 없을 수 있습니다 → "Windows 하이퍼바이저 플랫폼"만 체크해도 에뮬레이터 구동에는 충분합니다.

## Step 2. HAXM 제거 (설치되어 있는 경우)

❓ **왜?** 구형 가속기 HAXM은 **Hyper-V와 충돌**합니다. 둘이 공존하면 에뮬레이터가 무한 부팅하거나 BSOD가 날 수 있습니다.

🖱 **설정 → 앱 → 설치된 앱** → "Intel Hardware Accelerated Execution Manager" 검색 → 있으면 **제거**

```powershell
# 남아있는 서비스 확인 (없어야 정상)
sc query intelhaxm
```

✅ **예상 결과:** `지정된 서비스가 설치되어 있지 않습니다` — 이 메시지가 나오면 통과입니다.

## Step 3. Android Studio 설치 + SDK 36(Android 16) 구성

🖱 [developer.android.com/studio](https://developer.android.com/studio)에서 최신 안정판 다운로드 → 기본 옵션 설치 → 첫 실행 시 Standard 설정

🖱 **Android Studio → ⚙ Settings → Languages & Frameworks → Android SDK**
**[SDK Platforms]** ▸ **Android 16.0 (API 36)** 체크 / **[SDK Tools]** ▸ Platform-Tools ▸ Emulator ▸ Build-Tools 체크 → Apply

adb를 어디서든 쓸 수 있게 PATH를 등록합니다.

🖱 **시스템 환경 변수 편집 → 환경 변수 → Path → 새로 만들기** → `%LOCALAPPDATA%\Android\Sdk\platform-tools` 추가 → 확인 → **새 터미널** 열기

```bat
adb --version
```

✅ **예상 결과:** `Android Debug Bridge version 1.0.41` 이상 버전 문자열 출력

## Step 4. AVD ① — Google APIs x86_64 (adb root용)

❓ **왜?** 이미지 선택이 이 실습의 **가장 흔한 함정**입니다. **"Google Play" 이미지는 adb root가 막혀** Day 2부터의 모든 시스템 관찰이 불가능합니다. 반드시 **"Google APIs"** 이미지를 고르세요.

🖱 **Device Manager → ➕ Create Virtual Device** ▸ Pixel 6 → Next ▸ System Image: **API 36 · Target "Google APIs" · x86_64** → Download → Next ▸ 이름 `Pixel6_API36_GoogleAPIs` → Finish

⚠️ Target 열에 **"Google Play"**라고 적힌 이미지는 선택 금지! (▶ 아이콘에 Play 마크)

부팅 후 root 가능 여부를 즉시 검증합니다.

```bat
adb devices
adb root
adb shell whoami
```

✅ **예상 결과:** `restarting adbd as root` → `whoami` 출력이 **`root`**

## Step 5. AVD ② — Automotive Emulator (본 과정의 주 무대)

❓ **왜?** Car API·CarService·HVAC 실습(Day 3~4)은 **Automotive 프로필**에서만 가능합니다. 일반 폰 AVD에는 CarService가 없습니다.

🖱 **Device Manager → ➕ Create Virtual Device** ▸ Category: **Automotive** → 프로필 선택 → Next ▸ System Image: **API 36 · x86_64** → Download → Next ▸ 이름 `Automotive_API36` → Finish → ▶ 부팅

차량 런처가 뜨면, 이 과정의 핵심 서비스를 눈으로 확인합니다.

```bat
adb shell service list | findstr /i car
adb shell pidof com.android.car
```

✅ **예상 결과:** `car_service` 포함 **car_* 서비스 10여 개** + com.android.car의 **PID 숫자** — "차가 달려 있는 Android"임을 확인!

💡 두 에뮬레이터 동시 구동 시 `adb devices`로 시리얼 확인 후 `adb -s emulator-5554 shell ...`처럼 **-s 옵션**으로 대상 지정.

## Step 6. -writable-system 부팅 + remount (Day 2~4 대비 핵심)

❓ **왜?** Day 2 framework.jar 교체(6-5), Day 4 APEX 복사는 **/system 쓰기**가 필요하며, 이는 **부팅 시점에** `-writable-system`을 줘야만 가능합니다. GUI ▶ 버튼으로는 이 옵션을 줄 수 없습니다.

```bat
cd %LOCALAPPDATA%\Android\Sdk\emulator
emulator -list-avds
emulator -avd Automotive_API36 -writable-system
```

부팅이 끝나면 **새 터미널**에서:

```bat
adb root
adb remount
adb shell touch /system/WRITE_TEST && adb shell ls -l /system/WRITE_TEST
adb shell rm /system/WRITE_TEST
```

✅ **예상 결과:** `remount succeeded` → WRITE_TEST 생성·삭제 성공. (재부팅 요구 메시지 시 `adb reboot` 후 다시 root/remount)

⚠️ `-writable-system` 부팅 AVD는 스냅샷이 오염될 수 있습니다 → 문제 시 Device Manager → **Cold Boot Now**가 만능 탈출구 (실습 6-5 부팅 루프 대응과 동일).

## Step 7. 최종 리허설 — Day 1 관찰 명령 4종 미리 실행

❓ **왜?** Day 1 아침 '환경 삼중 점검(D1-0)'을 미리 통과해 두면 첫날을 온전히 학습에 쓸 수 있습니다.

```bat
adb shell ps -A | findstr /i "zygote system_server"
adb shell getprop ro.build.version.sdk
adb shell dumpsys activity | findstr /i "mFocused"
adb logcat -d -t 5
```

✅ **예상 결과:** zygote64·system_server 라인 / `36` / 포커스 정보 / 최근 로그 5줄 — 4개 모두 나오면 **이 실습의 모든 목표 달성!** 🎉

# 🏁 Pass 판정 체크리스트 (5/5 완료 시 통과)

- [ ] Hyper-V/WHPX 활성 + HAXM 부재 확인
- [ ] `adb --version`이 아무 폴더에서나 동작 (PATH 등록)
- [ ] Google APIs AVD에서 `adb root` → `whoami` = root
- [ ] Automotive AVD에서 `service list`에 car_* 서비스 확인
- [ ] `-writable-system` 부팅 → `remount succeeded` + /system 쓰기 테스트 성공

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| 에뮬레이터 검은 화면 멈춤 / BSOD | HAXM ↔ Hyper-V 충돌 | Step 2로 돌아가 HAXM 완전 제거 → 재부팅 |
| `adb root` → "cannot run as root in production builds" | **Google Play 이미지** 선택 | Step 4에서 **Google APIs** 이미지로 AVD 재생성 (최다 실수!) |
| `adb` 명령을 찾을 수 없음 | PATH 미등록 / 기존 터미널 재사용 | Step 3 PATH 확인 후 **터미널 새로 열기** |
| `adb remount` 실패 (read-only) | 일반 부팅(GUI ▶) 상태 | 명령줄에서 `-writable-system`으로 부팅 (Step 6) |
| 디바이스 offline / unauthorized | adb 서버 꼬임 | `adb kill-server && adb start-server` |
| 부팅 루프 등 상태 이상 | 스냅샷 오염 | Device Manager → **Cold Boot Now** |

# 🚗 현업 활용 포인트

💡 **이 환경이 곧 여러분의 '진단 콘솔'입니다.** 실무에서 차량 단말 이상 시 첫 동작이 정확히 오늘 한 것 — **adb 연결 → root → ps/service list/dumpsys로 계층 확인**입니다. `service list | grep car` 한 줄은 "CarService가 살아 있는가?"라는 워크숍 S2(DeadObjectException) 분석의 **첫 질문**이 되고, `-writable-system`+remount는 OEM 장비에서 framework 패치를 검증하는 일상 루틴과 동일합니다.

---

*Android Automotive Framework 교육 · 실습 D0-1 (36개 중 1번) · 다음: **D0-2 AOSP 최초 full build***
