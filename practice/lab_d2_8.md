# [D2-8] 실습 6-4 — SELinux 정책 + 14:40 전원 통합 빌드 시작

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★☆☆ | Day 2 Ch 6 마감 · ★전원 필수 | WinSCP + 서버 · 🎬 ⑱ 선시청 |

> 🎯 **실습 목표** — 서비스에 **전용 SELinux 라벨**을 부여하고 앱 접근을 허용한 뒤, 전원이 동시에 tmux 통합 빌드를 건다. "정책 없는 서비스는 조용히 실패한다"를 예방 접종.

## Step 1. 서비스 타입 선언 (.te)

🖱 `~/aosp/system/sepolicy/private/device_info_service.te` (신규):

```text
type device_info_service, system_api_service, system_server_service, service_manager_type;
```

## Step 2. 이름→라벨 매핑 (service_contexts)

🖱 `~/aosp/system/sepolicy/private/service_contexts` 에 한 줄 추가:

```text
device_info                               u:object_r:device_info_service:s0
```

❓ **왜?** 이 줄이 없으면 tcontext가 `default_android_service`로 잡혀(🎬 ⑱) 앱의 find가 거부됩니다 — "조용한 실패"의 진원지.

## Step 3. 앱 도메인 접근 허용

🖱 `~/aosp/system/sepolicy/private/untrusted_app.te` (또는 실습용 앱 도메인)에:

```text
allow untrusted_app device_info_service:service_manager find;
```

💡 binder call 은 대개 상속 규칙으로 커버되지만, 검증(6-5)에서 denied가 나오면 audit2allow(D2-11)로 보강합니다.

## Step 4. ★14:40 — 전원 통합 빌드 시작 (tmux)

```bash
tmux new -s build  # 이미 있으면 attach
cd ~/aosp && source build/envsetup.sh && lunch sdk_car_x86_64-userdebug
m framework-minus-apex services -j$(nproc) 2>&1 | tee ~/build_6x.log
# Ctrl+b, d 로 분리 → 오후 2교시(JNI/SELinux) 병행
```

✅ **예상 결과:** 빌드 가동 상태로 detach — 진행은 `tmux attach -t build` 로 수시 확인

# 🏁 Pass 판정 체크리스트

- [ ] .te 타입 선언 + service_contexts 한 줄 + allow 규칙 (3파일)
- [ ] sepolicy 문법 오류 없이 빌드 진입 (초반 sepolicy 단계 통과)
- [ ] tmux 세션에 빌드 가동·detach

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| sepolicy 컴파일 에러 | 세미콜론/속성명 오타 | 에러 라인의 파일 재확인 — 자료 원문과 대조 |
| neverallow 충돌 | 과한 allow | 규칙 축소, D2-11 절차로 재설계 |
| 빌드가 aidl에서 실패 | 6-1~6-3 미완 | 해당 실습으로 복귀 후 재빌드(증분) |

# 🚗 현업 활용 포인트

💡 벤더 서비스 추가 시 SELinux 3종(타입/컨텍스트/allow)은 **코드와 같은 PR에** 들어가야 합니다. "기능은 됐는데 보안팀에서 막힘"의 대부분이 오늘 이 15분을 건너뛴 결과입니다.

---
*실습 D2-8 (19/36) · 다음: **D2-9 JNI_test First~Fourth***
