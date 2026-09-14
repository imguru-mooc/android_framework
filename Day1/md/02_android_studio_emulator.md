# 실습 2. Android Studio · Emulator 설치

> **소요시간:** 20분 · **난이도:** ★☆☆
> **환경:** Windows 11 · Emulator API 35 (Google APIs)

## 목표

- Android Studio와 SDK를 설치한다.
- `adb root` 가 가능한 **Google APIs** 시스템 이미지(API 35)로 Automotive AVD를 만든다.
- `adb` 를 어느 cmd 창에서든 실행할 수 있게 PATH를 등록한다.

---

## Step 1. Android Studio 설치

1. https://developer.android.com/studio 에서 Windows용 설치 파일 다운로드
2. 설치 마법사 기본값으로 진행 (Android Virtual Device 체크 유지)
3. 첫 실행 → **Standard** 설정 → SDK 다운로드 완료까지 대기

## Step 2. AVD 생성 (Device Manager)

**Tools → Device Manager → Create Virtual Device (+)**

| 순서 | 설정 |
|---|---|
| 1 | Category **Automotive** → `Automotive (1408p landscape)` 선택 → Next |
| 2 | System Image → 기본 선택된 **API 35** (Google APIs, x86_64) 그대로 사용 → 이미지 옆 다운로드 아이콘 클릭 → Next |
| 3 | AVD Name : `Automotive_1408p_landscape` (이후 배치 파일에서 이 이름을 사용) |
| 4 | **Show Advanced Settings** → RAM 4096 MB, Internal Storage 8 GB, Graphics Hardware |
| 5 | Finish |

> ⚠ Automotive 카테고리에는 API 36 이미지가 아직 제공되지 않는다. 기본 선택인 **API 35** 를 사용한다. 실습 앱은 `minSdk 28` 이상이면 그대로 동작한다.
> ⚠ 이미지 이름에 **Google Play** 가 붙은 것은 `adb root` 가 막혀 있다. **Google APIs** 이미지인지 확인한다.

Automotive 이미지가 없으면 Phone 카테고리 → Pixel 8 + API 35 Google APIs 로 대체한다.

## Step 3. Emulator 실행 확인

Device Manager에서 ▶ 클릭 → 부팅 → Car Launcher(또는 홈 화면) 표시 확인.

## Step 4. PATH 등록

1. Windows 검색 → **시스템 환경 변수 편집** → **환경 변수**
2. 사용자 변수 **Path** → **편집** → **새로 만들기**
3. `%LOCALAPPDATA%\Android\Sdk\platform-tools` 추가
4. `%LOCALAPPDATA%\Android\Sdk\emulator` 추가
5. 확인 후 **새 cmd 창**을 열어 확인

```bat
adb version
emulator -list-avds
```

---

## 확인 포인트

- [ ] `adb version` 출력
- [ ] `emulator -list-avds` 에 `Automotive_1408p_landscape` 표시
- [ ] AVD 부팅 후 `adb devices` 에 `emulator-5554 device` 표시

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| Emulator가 매우 느리거나 시작 안 됨 | BIOS 가상화(VT-x) 활성화, Windows Hypervisor Platform 설치 |
| `adb` 를 찾을 수 없음 | PATH 등록 후 cmd 창을 새로 연다 |
| AVD 저장 공간 부족 | Advanced Settings → Internal Storage 늘리기 |
