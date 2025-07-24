# Android 14 간단한 APEX 빌드 실습 가이드 (sdk_car_x86_64)

## 1. 개발 환경 준비

### 필수 요구사항
- Android 14 (API Level 34) AOSP 소스 코드
- sdk_car_x86_64 타겟 환경
- Linux 개발 환경

### 환경 설정
```bash
# AOSP 빌드 환경 설정
source build/envsetup.sh
lunch sdk_car_x86_64
```

## 2. 프로젝트 구조

```
simple_apex/
├── Android.bp
├── apex_manifest.json
├── binary/
│   ├── Android.bp
│   └── main.cpp
├── com.example.simple.pem
├── com.example.simple.x509.pem
├── com.example.simple.pk8
└── com.example.simple.avbpubkey
```

## 3. 소스 코드 작성

### 3.1 간단한 실행 파일

**binary/main.cpp**
```cpp
#include <iostream>

int main() {
    std::cout << "Hello from Simple APEX!" << std::endl;
    std::cout << "This is a minimal APEX example" << std::endl;
    return 0;
}
```

### 3.2 바이너리 빌드 설정

**binary/Android.bp**
```json
cc_binary {
    name: "simple_apex_binary",
    srcs: ["main.cpp"],
    apex_available: ["com.example.simple"],
    min_sdk_version: "34",
}
```

### 3.3 APEX 매니페스트

**apex_manifest.json**
```json
{
    "name": "com.example.simple",
    "version": 1,
    "versionName": "1.0.0"
}
```

### 3.4 메인 APEX 빌드 설정

**Android.bp**
```json
apex {
    name: "com.example.simple",
    manifest: "apex_manifest.json",
    binaries: ["simple_apex_binary"],
    key: "com.example.simple.key",
    certificate: ":com.example.simple.certificate",
    min_sdk_version: "34",
}

apex_key {
    name: "com.example.simple.key",
    public_key: "com.example.simple.avbpubkey",
    private_key: "com.example.simple.pem",
}

android_app_certificate {
    name: "com.example.simple.certificate",
    certificate: "com.example.simple",
}
```

## 4. 인증서 생성

```bash
cd simple_apex

# 1. 4096비트 RSA 개인키 생성 (중요: 4096비트 필수!)
openssl genrsa -out com.example.simple.pem 4096

# 2. X.509 인증서 생성
openssl req -new -x509 -key com.example.simple.pem -out com.example.simple.x509.pem -days 365 \
    -subj "/C=KR/ST=Seoul/L=Seoul/O=SimpleApex/OU=Development/CN=com.example.simple"

# 3. 개인키를 PKCS#8 DER 형식으로 변환
openssl pkcs8 -topk8 -outform DER -in com.example.simple.pem -inform PEM -out com.example.simple.pk8 -nocrypt

# 4. AVB 공개키 생성
pip install avbtool
avbtool extract_public_key --key com.example.simple.pem --output com.example.simple.avbpubkey

# 5. 파일 및 키 크기 확인
ls -la com.example.simple.*
echo "Checking key size:"
openssl rsa -in com.example.simple.pem -text -noout | grep "Private-Key"
```

## 5. SELinux 정책 설정

```bash
# AOSP 루트 디렉토리에서 실행
cd $ANDROID_BUILD_TOP

# SELinux 정책 디렉토리 생성
mkdir -p system/sepolicy/apex

# 간단한 파일 컨텍스트 생성
cat > system/sepolicy/apex/com.example.simple-file_contexts << 'EOF'
(/.*)?                                          u:object_r:system_file:s0
/bin(/.*)?                                      u:object_r:system_file:s0
/bin/simple_apex_binary                         u:object_r:system_file:s0
EOF
```

## 6. 빌드 및 테스트

### 6.1 빌드
```bash
cd simple_apex

# 빌드 환경 설정
source build/envsetup.sh
lunch sdk_car_x86_64

# 개별 바이너리 빌드 테스트
cd simple_apex

mm

# 빌드 결과 확인
ls -la out/target/product/sdk_car_x86_64/system/apex/com.example.simple.apex
```

### 6.2 에뮬레이터에서 테스트
```bash

# APEX 파일 푸시
adb push com.example.simple.apex /data

# 에뮬레이터에서 설치 (루트 권한 필요)
adb shell
su
mount -o remount,rw /system
cp /data/com.example.simple.apex /system/apex/
reboot

# 재부팅 후 테스트
adb shell ls -la /apex/com.example.simple/
adb shell /apex/com.example.simple/bin/simple_apex_binary
```

