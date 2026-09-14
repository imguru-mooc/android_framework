# 실습 3. adb 접속 방법

> **소요시간:** 15분 · **난이도:** ★☆☆
> **환경:** Windows cmd + Emulator (API 36, Google APIs)

## 목표

- `adb` 로 Emulator에 접속하고 root 권한을 얻는다.
- 파일 전송, 로그 확인, System Service 덤프 등 이후 실습의 공통 명령을 익힌다.

---

## Step 1. 연결 확인

```bat
adb devices
```

```text
List of devices attached
emulator-5554   device
```

`offline` 이나 목록이 비어 있으면:

```bat
adb kill-server
adb start-server
adb devices
```

## Step 2. root 권한

```bat
adb root
```

```text
restarting adbd as root
```

```bat
adb shell id
```

```text
uid=0(root) gid=0(root) ...
```

> Google APIs / userdebug 이미지에서만 동작한다. `adbd cannot run as root in production builds` 가 나오면 이미지가 잘못됐다.

## Step 3. 쉘 진입과 종료

```bat
adb shell
```

```bash
emulator_car64_x86_64:/ # getprop ro.build.version.release
16
emulator_car64_x86_64:/ # getprop ro.build.version.sdk
36
emulator_car64_x86_64:/ # exit
```

한 줄 명령은 쉘에 들어가지 않고 바로 실행한다.

```bat
adb shell getprop ro.product.model
adb shell "ps -A | grep zygote"
```

## Step 4. 파일 전송

```bat
adb push my_binary /data
adb shell chmod 755 /data/my_binary
adb shell /data/my_binary

adb pull /sdcard/screenshot.png .
```

## Step 5. /system 쓰기 (필요 시)

```bat
adb root
adb remount        REM 최초 1회: "reboot required" 표시
adb reboot
adb root
adb remount        REM "remount succeeded" 확인
```

## Step 6. 로그와 덤프

```bat
adb logcat -s CalcService CalcClient      REM 태그 필터
adb logcat -c                             REM 로그 버퍼 비우기
adb shell dumpsys activity | more
adb shell dumpsys meminfo com.android.systemui
adb shell service list
```

## Step 7. 앱 설치/제거

```bat
adb install -r app-debug.apk
adb uninstall com.example.binderlab
```

---

## 명령 요약

| 명령 | 용도 |
|---|---|
| `adb devices` | 연결 확인 |
| `adb root` / `adb unroot` | root 전환 / 해제 |
| `adb shell [cmd]` | 쉘 진입 / 한 줄 실행 |
| `adb push` / `adb pull` | 파일 전송 |
| `adb remount` | `/system` 쓰기 가능 |
| `adb logcat -s TAG` | 로그 필터 |
| `adb shell dumpsys <svc>` | Service 상태 덤프 |
| `adb kill-server` / `start-server` | 데몬 재시작 |

## 확인 포인트

- [ ] `adb shell id` → `uid=0(root)`
- [ ] `adb shell getprop ro.build.version.sdk` → `36`
- [ ] 임의 파일 `adb push` → `adb shell ls -la /data/<file>` 확인
