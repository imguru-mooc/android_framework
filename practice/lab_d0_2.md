# [D0-2] AOSP 최초 full build — 빌드 서버 (tmux)

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★☆☆☆ | Day 0 — 교육 1주 전까지 ★전원 필수 | 빌드 서버 계정(stuNN) · PuTTY(D0-3) |

> 🎯 **실습 목표** — 빌드 서버에서 `sdk_car_x86_64-userdebug` 타겟의 **최초 full build를 완주**하고, tmux 세션 안에서 빌드를 돌리는 습관을 몸에 붙인다. 이 빌드가 없으면 Day 2 오후부터의 모든 AOSP 실습이 불가능하다.

## Step 1. 서버 접속 및 소스 확인

🖱 PuTTY로 저장된 세션(D0-3)에 접속합니다.

```bash
ls ~/aosp
# .repo  Android.bp  art  bionic  build  frameworks  ...  (운영측이 미리 sync 완료)
df -h ~ | tail -1
```

✅ **예상 결과:** `~/aosp`에 AOSP 트리 존재, 홈 여유 공간 **250GB 이상**

## Step 2. 빌드 환경 초기화 (envsetup + lunch)

```bash
cd ~/aosp
source build/envsetup.sh
lunch sdk_car_x86_64-userdebug
```

✅ **예상 결과:** `TARGET_PRODUCT=sdk_car_x86_64`, `TARGET_BUILD_VARIANT=userdebug` 요약 표 출력

⚠️ 타겟 이름 오타가 가장 흔한 실수 — `lunch` 만 입력하면 번호 목록에서 고를 수 있습니다. **car**가 들어간 userdebug인지 반드시 확인.

## Step 3. tmux 세션 안에서 빌드 시작

❓ **왜?** full build는 1~3시간. SSH가 끊기면 빌드도 죽습니다 — **tmux 세션이 생명줄**입니다.

```bash
tmux new -s build
# (tmux 안에서)
cd ~/aosp && source build/envsetup.sh && lunch sdk_car_x86_64-userdebug
m -j$(nproc) 2>&1 | tee ~/build_full.log
```

💡 분리: `Ctrl+b` 누른 뒤 `d` / 재접속: `tmux attach -t build` / 목록: `tmux ls` — 이 3개가 4일 내내 쓰는 전부입니다.

## Step 4. 진행 확인과 재접속 리허설

빌드 도중 **일부러 PuTTY 창을 닫고**, 다시 접속해 복귀해 봅니다.

```bash
tmux attach -t build
tail -f ~/build_full.log   # 다른 창에서 진행 로그 보기 (Ctrl+C로 나가도 빌드는 계속)
```

✅ **예상 결과:** 창을 닫아도 빌드가 살아 있고, attach로 그대로 복귀

## Step 5. 완료 확인

```bash
ls -lh ~/aosp/out/target/product/emulator_car64_x86_64/ | grep -E "system.img|framework"
echo $?
```

✅ **예상 결과:** `#### build completed successfully ####` 로그 + `system.img` 존재. 이 경로가 앞으로 계속 쓸 **$OUT**입니다.

# 🏁 Pass 판정 체크리스트

- [ ] lunch 요약에 `sdk_car_x86_64` + `userdebug` 확인
- [ ] tmux 세션에서 빌드 시작, detach/attach 왕복 성공
- [ ] build completed successfully 로그 확보
- [ ] `$OUT/system.img` 존재 확인

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| ninja 도중 OOM/서버 버벅임 | 코어 수 대비 메모리 부족 | `m -j8` 처럼 병렬도 낮춰 재시작 (증분이라 이어서 진행) |
| `Could not find platform/...` | envsetup/lunch 없이 m 실행 | Step 2부터 다시 (새 셸마다 필수) |
| 디스크 풀 | out/ 용량 폭증 | 조교에게 할당량 요청, 불필요 로그 삭제 |
| 접속 끊김 후 빌드 사라짐 | tmux 밖에서 실행함 | 반드시 Step 3 순서로 — tmux가 먼저 |

# 🚗 현업 활용 포인트

💡 OEM 개발의 하루는 "빌드 서버에 걸어두고 다른 일"의 반복입니다. **tmux(또는 screen) 없는 원격 빌드는 없다**고 기억하세요. `tee`로 로그를 남기는 습관은 빌드 실패 원인 공유(팀 채널 첨부)의 기본기입니다.

---
*실습 D0-2 (36개 중 2번) · 다음: **D0-3 PuTTY 설정***
