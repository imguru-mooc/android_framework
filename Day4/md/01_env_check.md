# 실습 1. Day 4 환경 점검 — writable-system · 2단계 remount · aapt · 실습 디렉토리

> **소요시간:** 15분 · **난이도:** ★☆☆
> **환경:** Windows 11 (adb · Android Studio · WinSCP) + 커스텀 Emulator + 빌드 서버

## 목표

- Emulator 를 `-writable-system` 으로 띄우고 **2단계 `adb remount`** 를 끝내 `/system` 에 쓸 수 있게 한다 (실습 2·6·7·11 의 전제).
- `aapt` 를 PATH 에 넣는다 (실습 6).
- 빌드 서버에 `simple_apex/`, `RRO_test/overlay_list/` 를 만들고 Android Studio 프로젝트 zip 3종을 푼다.

---

## Step 1. Emulator 를 writable-system 으로 실행 (Windows)

Day 1 의 `run_custom_car_emulator.bat` 에 `-writable-system` 이 있는지 확인한다.

```bat
%EMULATOR% -sysdir %IMAGES% -avd %AVD_NAME% -writable-system -no-snapshot-load -no-snapshot-save ^
  -qemu -append "androidboot.selinux=permissive"
```

## Step 2. 2단계 remount

```bat
adb root
adb remount
```

```text
Successfully disabled verity
Using overlayfs for /system
Using overlayfs for /vendor
...
Now reboot your device for settings to take effect
```

```bat
adb reboot
adb wait-for-device
adb root
adb remount
```

```text
remount succeeded          ← ★ 이것이 나와야 /system 에 쓸 수 있다
```

```bash
adb shell
touch /system/remount_ok && ls -l /system/remount_ok && rm /system/remount_ok
exit
```

| 단계 | 역할 |
|---|---|
| 1차 `adb remount` | dm-verity 비활성화 + overlayfs 설정 (재부팅 필요) |
| `adb reboot` | 설정 적용 |
| 2차 `adb remount` | **실제로 /system 을 쓰기 가능으로 마운트** — 이후 재부팅해도 유지 |

2차 remount 를 빼먹으면 이후 모든 `adb push /system/...` 이 `Read-only file system` 이다.

## Step 3. aapt PATH (실습 6)

```bat
dir /s /b %LOCALAPPDATA%\Android\Sdk\build-tools\aapt.exe
set PATH=%PATH%;%LOCALAPPDATA%\Android\Sdk\build-tools\35.0.0
aapt version
```

## Step 4. 빌드 서버 디렉토리

```bash
cd ~/android
mkdir -p simple_apex/binary RRO_test/overlay_list
ls system/sepolicy/apex/ 2>/dev/null || mkdir -p system/sepolicy/apex
ls simple_apex/*.pem simple_apex/*.pk8 simple_apex/*.avbpubkey      # ★ 강사가 미리 넣어 둔 키 4종 확인
```

키가 없으면 실습 2 Step 3 의 명령으로 직접 만든다 (2분).

## Step 5. Android Studio 프로젝트

`C:\aosp16\day4\` 에 저장소 zip 3종을 푼다: `FloatingOverlayDemo`, `SplitScreenDemo`, `HvacSimulator`. 각각 Android Studio 로 열어 Gradle sync 만 해 둔다.

## Step 6. Emulator 확인

```bash
adb shell
getprop ro.build.type                       # userdebug
getenforce                                  # Permissive
pm list packages | grep -E "kitchensink|systemui"
service list | grep -E "car_service|vehicle|overlay"
exit
```

---

## 확인 포인트

- [ ] `adb remount` 두 번째 결과 `remount succeeded`, `/system` 에 touch 성공
- [ ] `aapt version` 출력
- [ ] `simple_apex/` 에 키 4종
- [ ] `car_service`, `IVehicle/default`, `overlay` 서비스 존재

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `remount` 가 계속 `reboot required` | `-writable-system` 없이 부팅됨 → bat 확인 후 재실행 |
| `Read-only file system` | 2차 `adb root; adb remount` 누락 |
| `aapt` 없음 | SDK Manager → SDK Tools → Android SDK Build-Tools 설치 |
