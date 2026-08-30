# [D4-6] Lab 4-3 — simple APEX 빌드·설치·실행

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★★☆ | Day 4 오후 1교시 · ★전원 필수 | 서버(apex_build_tutorial.md 배포 코드)+WinSCP+로컬 · 🎬 ⑩ 선시청 |

> 🎯 **실습 목표** — 최소 APEX(`com.example.simple`)를 직접 빌드해 에뮬레이터에 얹고, `/apex/` 마운트 아래의 바이너리를 실행한다. 키·서명·sepolicy까지 전 과정을 손으로.

## Step 1. 프로젝트 구조 확인 (vendor/edu/simple_apex/)

```text
simple_apex/
├── Android.bp            # apex + apex_key + certificate
├── apex_manifest.json    # {"name":"com.example.simple","version":1,...}
├── binary/ (main.cpp: "Hello from Simple APEX!" 출력, Android.bp: cc_binary)
└── (Step 2에서 만들 키 4종)
```

## Step 2. 인증서·키 4종 생성 (서버, simple_apex/에서)

```bash
openssl genrsa -out com.example.simple.pem 4096      # ★ 4096비트 필수!
openssl req -new -x509 -key com.example.simple.pem -out com.example.simple.x509.pem -days 365 \
  -subj "/C=KR/ST=Seoul/L=Seoul/O=SimpleApex/OU=Dev/CN=com.example.simple"
openssl pkcs8 -topk8 -outform DER -in com.example.simple.pem -inform PEM \
  -out com.example.simple.pk8 -nocrypt
avbtool extract_public_key --key com.example.simple.pem --output com.example.simple.avbpubkey
openssl rsa -in com.example.simple.pem -text -noout | grep "Private-Key"
```

✅ **예상 결과:** 파일 4종 생성 + `Private-Key: (4096 bit ...)` 확인

## Step 3. SELinux file_contexts

```bash
mkdir -p ~/aosp/system/sepolicy/apex
cat > ~/aosp/system/sepolicy/apex/com.example.simple-file_contexts << 'EOF'
(/.*)?                        u:object_r:system_file:s0
/bin(/.*)?                    u:object_r:system_file:s0
/bin/simple_apex_binary       u:object_r:system_file:s0
EOF
```

## Step 4. 빌드 → 산출물 확인

```bash
cd ~/aosp && source build/envsetup.sh && lunch sdk_car_x86_64-userdebug
cd vendor/edu/simple_apex && mm
ls -lh $OUT/system/apex/ | grep simple
```

✅ **예상 결과:** `com.example.simple.apex` 생성

## Step 5. 전송 → 설치(시스템 복사 방식) → 실행

🖱 WinSCP **apex** 북마크 → `com.example.simple.apex` 다운로드. 로컬에서:

```bat
adb root & adb remount
adb push com.example.simple.apex /data/
adb shell "cp /data/com.example.simple.apex /system/apex/ && sync"
adb reboot
adb wait-for-device
adb shell ls /apex/ | findstr simple
adb shell /apex/com.example.simple/bin/simple_apex_binary
```

✅ **예상 결과:**

```text
com.example.simple
com.example.simple@1
Hello from Simple APEX!
This is a minimal APEX example
```

## Step 6. (실패 시) apexd 진단

```bat
adb logcat -s apexd | findstr /i "simple fail verif"
adb shell pm get-stagedsessions
```

| 에러 | 의미 | 조치 |
|---|---|---|
| VERIFICATION_FAILURE | 서명/키 불일치 | Step 2 재수행(4096·pk8·avbpubkey 세트) |
| VERSION_DOWNGRADE | 낮은 version 설치 시도 | apex_manifest version 올리기 |
| 마운트 안 됨 | file_contexts 누락 | Step 3 + 재빌드 |
| 최후 수단 | 오염된 활성본 | `/data/apex/active/` 제거 후 재부팅 |

# 🏁 Pass 판정 체크리스트

- [ ] 키 4종 + 4096bit 확인
- [ ] $OUT/system/apex/에 .apex 생성
- [ ] /apex/ 마운트 확인 + 바이너리 실행 출력 캡처

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| avbtool 없음 | PATH | `~/aosp/external/avb/avbtool.py` 직접 호출 |
| mm이 apex를 안 만듦 | lunch 누락/디렉토리 밖 | Step 4 순서 준수 |
| reboot 후 /apex에 없음 | /system 복사 누락·sync 전 재부팅 | Step 5 재수행 |

# 🚗 현업 활용 포인트

💡 mainline 모듈(ART·미디어 등) 업데이트 이슈를 볼 때, 오늘 손으로 만진 **서명→staged/마운트→apexd 로그** 체인이 그대로 디버깅 경로입니다. "APEX가 안 올라와요"는 이제 3분 진단 항목.

---
*실습 D4-6 (35/36) · 다음: **D4-7 종합 트러블슈팅 워크숍***
