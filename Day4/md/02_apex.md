# 실습 2. APEX 만들기 — simple_apex

> **소요시간:** 35분 · **난이도:** ★★☆ · **챕터:** Ch 1
> **디렉토리:** `~/android/simple_apex/` · 저장소 `apex_build_tutorial_android16.md`

## 목표

- 바이너리 하나를 담은 APEX 모듈을 만들어 빌드·설치하고 `/apex/` 에 마운트되는 것을 본다.
- `file_contexts` 위치 규칙과 `compressible` 규칙을 오류로 확인한다.

## 구조

```
~/android/
├── simple_apex/
│   ├── Android.bp                 apex · apex_key · android_app_certificate
│   ├── apex_manifest.json
│   ├── binary/Android.bp          cc_binary (apex_available)
│   ├── binary/main.cpp
│   ├── com.example.simple.pem / .pk8 / .x509.pem / .avbpubkey   ← 키 4종
└── system/sepolicy/apex/com.example.simple-file_contexts       ← ★ 반드시 여기
```

---

## Step 1. 바이너리

**`simple_apex/binary/main.cpp`**

```cpp
#include <iostream>
#include <unistd.h>

int main() {
    std::cout << "Hello from Simple APEX on Android 16 (Baklava)!" << std::endl;
    std::cout << "pid=" << getpid() << " running from /apex/com.example.simple/bin" << std::endl;
    return 0;
}
```

**`simple_apex/binary/Android.bp`**

```text
cc_binary {
    name: "simple_apex_binary",
    srcs: ["main.cpp"],
    apex_available: ["com.example.simple"],   // ★ 이 APEX 에 포함 허용
    min_sdk_version: "36",
}
```

## Step 2. 매니페스트와 APEX 모듈

**`simple_apex/apex_manifest.json`**

```json
{
    "name": "com.example.simple",
    "version": 1,
    "versionName": "1.0.0"
}
```

**`simple_apex/Android.bp`**

```text
apex {
    name: "com.example.simple",
    manifest: "apex_manifest.json",
    // file_contexts 는 명시하지 않는다 → system/sepolicy/apex/com.example.simple-file_contexts 자동 탐색
    binaries: ["simple_apex_binary"],
    key: "com.example.simple.key",
    certificate: ":com.example.simple.certificate",
    min_sdk_version: "36",
    updatable: false,          // 커스텀 APEX — Mainline 업데이트 대상 아님
    // compressible 은 쓰지 않는다 (updatable: false 와 함께 true 면 빌드 오류)
}

apex_key {
    name: "com.example.simple.key",
    public_key: "com.example.simple.avbpubkey",
    private_key: "com.example.simple.pem",
}

android_app_certificate {
    name: "com.example.simple.certificate",
    certificate: "com.example.simple",       // com.example.simple.pk8 + .x509.pem
}
```

## Step 3. 키 4종 (강사가 미리 넣어 두었으면 확인만)

```bash
cd ~/android/simple_apex
ls com.example.simple.*
```

없을 때만:

```bash
openssl genrsa -out com.example.simple.pem 4096
openssl req -new -x509 -key com.example.simple.pem -out com.example.simple.x509.pem -days 3650 \
    -subj "/C=KR/ST=Seoul/L=Seoul/O=SimpleApex/OU=Dev/CN=com.example.simple"
openssl pkcs8 -topk8 -outform DER -in com.example.simple.pem -inform PEM -out com.example.simple.pk8 -nocrypt
avbtool extract_public_key --key com.example.simple.pem --output com.example.simple.avbpubkey
```

## Step 4. file_contexts — 반드시 system/sepolicy/apex/

```bash
cd ~/android
cat > system/sepolicy/apex/com.example.simple-file_contexts << 'EOT'
(/.*)?                                          u:object_r:system_file:s0
/bin(/.*)?                                      u:object_r:system_file:s0
/bin/simple_apex_binary                         u:object_r:system_file:s0
EOT
```

## Step 5. 빌드

```bash
cd ~/android
m com.example.simple
ls -la $OUT/system/apex/com.example.simple.apex
deapexer list $OUT/system/apex/com.example.simple.apex      # ./bin/simple_apex_binary, apex_manifest.pb …
```

## Step 6. 설치 — WinSCP → Windows

**WinSCP** — `out/target/product/emulator_car64_x86_64/system/apex/com.example.simple.apex` → `C:\aosp16\bin\`

```bat
adb root
adb remount
adb push C:\aosp16\bin\com.example.simple.apex /system/apex/
adb reboot
adb wait-for-device
```

```bash
adb shell
ls -la /apex/com.example.simple/                 # bin/  apex_manifest.pb  etc/
ls -la /apex/ | grep simple                       # com.example.simple  com.example.simple@1  ← 버전 심볼릭
/apex/com.example.simple/bin/simple_apex_binary
# Hello from Simple APEX on Android 16 (Baklava)!
mount | grep simple                              # /dev/block/loop… on /apex/com.example.simple@1 type ext4 (ro)
dumpsys apexservice | grep -A6 com.example.simple
logcat -d -s apexd | tail -5
exit
```

## Step 7. 오류 재현 (각 1분)

① `file_contexts` 를 `simple_apex/` 안에 두고 `Android.bp` 에 `file_contexts: "file_contexts"` 를 추가 → `m com.example.simple`

```text
error: module "com.example.simple": file_contexts: should be under system/sepolicy, but found in "simple_apex/"
```

② `apex {}` 에 `compressible: true` 추가 → 빌드

```text
error: compressible: do not compress non-updatable APEX
```

둘 다 원복한다.

---

## 확인 포인트

- [ ] `$OUT/system/apex/com.example.simple.apex` 생성, `deapexer list` 에 바이너리
- [ ] 재부팅 후 `/apex/com.example.simple@1` 마운트, 바이너리 실행
- [ ] 오류 ①② 메시지 확인 후 원복

## 핵심 정리

| 항목 | 의미 |
|---|---|
| APEX | 파일시스템 이미지(ext4)+manifest+서명 을 `.apex` 로 묶은 모듈. 부팅 시 `apexd` 가 loop mount → `/apex/<name>@<ver>` |
| `apex_key` / `android_app_certificate` | AVB 서명(부팅 검증) + APK 서명(패키지 검증) 두 벌 |
| `file_contexts` 위치 | `system/sepolicy/apex/<name>-file_contexts` 고정 — SELinux 라벨은 정책 트리에서만 |
| `updatable: false` | 이미지에 내장되는 비-Mainline 모듈. `compressible` 불가 |
| Day 2 와의 관계 | APEX 안의 바이너리도 init.rc 로 띄우고 SELinux 도메인을 줄 수 있다 (실습 9 규칙 동일) |

## 트러블슈팅

| 증상 | 해결 |
|---|---|
| `avbtool: not found` | `lunch` 후 PATH 에 들어온다. 또는 `external/avb/avbtool.py` |
| `Private-Key: (2048 bit)` | 4096 비트 필수 — 키 재생성 |
| `/apex/com.example.simple` 없음 | `logcat -s apexd` 에서 verify 실패 확인 — 키 불일치, `adb push` 경로 확인, reboot 여부 |
