# [D2-11] SELinux 실습 — avc denied 디버깅 5단계

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★☆☆ | Day 2 오후 2교시 · ★전원 필수 | 서버(sepolicy) + 로컬 에뮬레이터 · 🎬 ⑱ 선시청 |

> 🎯 **실습 목표** — 실제 denied 로그를 수집→분석→audit2allow→neverallow 확인→반영의 **5단계 절차**로 처리하는 근육을 만든다. 6-5 검증에서 denied가 나올 때의 표준 대응.

## Step 1. denied 로그 수집

```bat
adb shell "dmesg | grep 'avc:.*denied' | tail -10"
adb logcat -d | findstr /i "avc denied"
```

✅ **예상 결과:** 한 줄 이상 확보(없으면 강사 제공 트리거 앱 실행). 예:

```text
avc: denied { find } for service=device_info scontext=u:r:untrusted_app:s0 tcontext=u:object_r:default_android_service:s0 tclass=service_manager
```

## Step 2. 4요소 분석 (지면)

| 요소 | 값 | 의미 |
|---|---|---|
| permission | { find } | 무엇을 하려다 |
| scontext | untrusted_app | 누가 |
| tcontext | default_android_service | 무엇에 — **default_* = 전용 라벨 없음 신호!** |
| tclass | service_manager | 어떤 종류의 판정 |

## Step 3. audit2allow로 초안 생성 (서버)

```bash
adb shell "dmesg | grep 'avc:.*denied'" > /tmp/avc.txt   # 로컬→서버로 옮겨도 OK
cat /tmp/avc.txt | audit2allow -p ~/aosp/out/target/product/emulator_car64_x86_64/root/sepolicy
```

✅ **예상 결과:** `allow untrusted_app ...:service_manager find;` 류 초안 — ⚠️ **그대로 복붙 금지**, 다음 단계 필수

## Step 4. neverallow 충돌 확인

```bash
grep -rn "neverallow" ~/aosp/system/sepolicy/public/  | grep -i "service_manager\|binder" | head -5
grep -rn "neverallow" ~/aosp/system/sepolicy/private/ | grep -i "service_manager\|binder" | head -5
```

✅ **예상 결과:** 충돌 없음 → 최소 범위로 .te 반영 / 충돌 있음 → **아키텍처 재검토**(라벨 재설계, 접근 경로 변경)

## Step 5. 반영→빌드→재검증 + 흐름도 완성

```text
① 수집(dmesg/logcat) → ② 4요소 분석 → ③ audit2allow 초안
→ ④ neverallow 충돌 확인 → ⑤ .te 반영 → 빌드 → denied 재확인(없어야 통과)
```

# 🏁 Pass 판정 체크리스트

- [ ] denied 원문 1건 + 4요소 표 완성
- [ ] audit2allow 초안 생성 + neverallow grep 수행
- [ ] 5단계 흐름도 자필 완성 ("default_* 발견 시 첫 조치 = service_contexts"를 명시)

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| audit2allow: command not found | 호스트 도구 미설치 | 조교 요청(`setools`), 또는 강사 데모 화면 참조 |
| -p sepolicy 경로 오류 | out 경로 상이 | `find ~/aosp/out -name sepolicy | head` 로 실제 경로 확인 |

# 🚗 현업 활용 포인트

💡 vendor 이미지 교체 후 "권한은 있는데 안 돼요"의 80%가 라벨 문제입니다. **tcontext에 default_ 가 보이면 규칙 추가가 아니라 라벨 부여**가 첫 수 — 오늘 표의 3행이 그 판별식입니다.

---
*실습 D2-11 (22/36) · 다음: **D2-12 실습 6-5 검증 착수***
