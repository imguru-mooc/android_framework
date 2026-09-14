# 실습 4. 빌드 서버에서 Android 빌드

> **소요시간:** 30분 · **난이도:** ★★☆
> **환경:** Ubuntu 24 빌드 서버 (PuTTY 접속) + Windows (WinSCP, Emulator)

## 목표

- AOSP 빌드 환경(`envsetup.sh`, `lunch`)을 설정한다.
- 전체 빌드(`m`)로 Emulator 이미지를 생성한다.
- 생성한 커스텀 시스템 이미지를 Windows로 가져와 Emulator를 띄운다.
- 모듈 단위 빌드(`mm`)로 결과물을 만들고 커스텀 Emulator에서 실행한다.

> AOSP 전체 빌드는 수 시간이 걸리므로 각 수강생 계정에 **사전 빌드가 완료된 소스 트리**가 준비되어 있다. 실습에서는 증분 전체 빌드 → 이미지 생성 → 커스텀 Emulator 실행 → 모듈 빌드 순으로 모두 직접 수행한다.

---

## Step 1. 소스 위치 확인

```bash
cd ~/aosp
ls
# Android.bp  build  device  frameworks  hardware  out  packages  system  ...
```

## Step 2. 빌드 환경 설정 (터미널 세션마다)

```bash
source build/envsetup.sh
lunch sdk_car_x86_64-aosp_current-userdebug
```

Android 16의 `lunch` 형식은 **product-release-variant** 3-파트다.

| 버전 | lunch 예 |
|---|---|
| Android 14 | `lunch sdk_car_x86_64-userdebug` |
| Android 16 | `lunch sdk_car_x86_64-aosp_current-userdebug` |

```bash
echo $OUT
# /home/worker/aosp/out/target/product/emulator_car64_x86_64
```

## Step 3. 전체 빌드와 Emulator 이미지 생성

```bash
cd ~/aosp
m -j$(nproc)          # 사전 빌드 완료 상태이므로 증분 빌드만 수행 (수 분 이내)
m emu_img_zip
ls $OUT/sdk-repo-linux-system-images.zip
```

전체 빌드가 끝나면 `$OUT/system.img`, `$OUT/ramdisk.img` 등 Emulator 이미지가 갱신되고, `m emu_img_zip` 이 이를 하나의 zip 으로 묶는다.

> 각자 계정에서 `m -j$(nproc)` 을 실행해 증분 빌드가 정상 종료(`#### build completed successfully ####`)되는 것까지 확인한다.

## Step 4. 커스텀 이미지로 Emulator 실행 (Windows)

1. WinSCP로 `out/target/product/emulator_car64_x86_64/sdk-repo-linux-system-images.zip` 을 `C:\aosp16` 에 다운로드 후 압축 해제 → `C:\aosp16\x86_64` 폴더에 `system.img`, `ramdisk.img`, `kernel-ranchu` 등이 있는지 확인
2. 실행 중인 Android Studio Emulator 가 있으면 종료한다 (같은 AVD 를 사용)
3. `C:\aosp16\run_custom_car_emulator.bat` 작성 (메모장 → 다른 이름으로 저장 → 파일 형식 "모든 파일")

```bat
@echo off
set EMULATOR=%LOCALAPPDATA%\Android\Sdk\emulator\emulator.exe
set AVD_NAME=Automotive_1408p_landscape
set IMAGES=C:\aosp16\x86_64

%EMULATOR% ^
-sysdir %IMAGES% ^
-avd %AVD_NAME% ^
-writable-system ^
-no-snapshot-load ^
-no-snapshot-save ^
-verbose ^
-show-kernel ^
-qemu -append "androidboot.selinux=permissive"
```

4. 더블클릭 실행 → 콘솔 창에 커널 로그가 흐르며 부팅 → 부팅 후 확인

```bat
adb shell getprop ro.build.fingerprint
adb shell getprop ro.build.type
```

`userdebug` 와 서버에서 빌드한 fingerprint가 보이면 커스텀 이미지가 올라온 것이다.

이후 실습(Native 바이너리 배포, 커널 모듈 로드)은 이 커스텀 이미지 Emulator 위에서 진행한다.

## Step 5. 모듈 단위 빌드 (`mm`)

```bash
mkdir -p ~/aosp/sp_test && cd ~/aosp/sp_test
```

**`main.cpp`**

```cpp
#include <cstdio>
#include <unistd.h>

int main() {
    printf("Hello AOSP! pid=%d uid=%d\n", getpid(), getuid());
    return 0;
}
```

**`Android.bp`**

```text
cc_binary {
    name: "my_sp",
    srcs: ["main.cpp"],
    cflags: ["-Wall", "-Werror"],
}
```

```bash
mm -j$(nproc)
ls $OUT/system/bin/my_sp
```

## Step 6. Windows로 가져와 Emulator에서 실행

1. WinSCP로 `out/target/product/emulator_car64_x86_64/system/bin/my_sp` 를 Windows 작업 폴더로 다운로드
2. cmd 창:

```bat
adb root
adb push my_sp /data
adb shell chmod 755 /data/my_sp
adb shell /data/my_sp
```

```text
Hello AOSP! pid=4321 uid=0
```

이 흐름(**`mm` → WinSCP → `adb push` → 실행**)이 이후 모든 Native 실습의 기본 배포 절차다.

---

## 명령 요약

| 명령 | 용도 |
|---|---|
| `source build/envsetup.sh` | 빌드 함수 로드 (세션마다) |
| `lunch <product>-<release>-<variant>` | 타겟 선택 |
| `m -j$(nproc)` | 전체 빌드 |
| `mm -j$(nproc)` | 현재 디렉토리 모듈만 빌드 |
| `m <module>` | 모듈명으로 빌드 |
| `m emu_img_zip` | Emulator 이미지 zip 생성 |
| `echo $OUT` | 출력 디렉토리 |

## 확인 포인트

- [ ] `echo $OUT` 경로 출력
- [ ] `m -j$(nproc)` 증분 빌드 성공, `sdk-repo-linux-system-images.zip` 생성
- [ ] 커스텀 이미지 Emulator 부팅 (`ro.build.type = userdebug`)
- [ ] `$OUT/system/bin/my_sp` 생성, Emulator에서 실행 성공

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `mm: command not found` | `source build/envsetup.sh` 를 다시 실행 |
| `lunch` 타겟 없음 | `lunch --print-all-targets \| grep car` 로 확인 |
| `-Werror` 로 빌드 실패 | 경고를 수정하거나 `cflags` 에서 제거 |
| Emulator가 기본 이미지로 부팅 | `-sysdir` 경로에 `system.img`, `ramdisk.img` 가 있는지 확인 |
