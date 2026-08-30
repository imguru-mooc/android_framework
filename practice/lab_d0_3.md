# [D0-3] PuTTY 설정 — 빌드 서버 원격 접속

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★☆☆☆☆ | Day 0 ★전원 필수 | 서버 주소/포트, 계정(stuNN) — 운영측 안내 메일 |

> 🎯 **실습 목표** — PuTTY 세션을 저장해 두 번 클릭으로 접속하고, keepalive로 끊김을 방지하며, 서버의 tmux 동작을 확인한다.

## Step 1. PuTTY 설치

🖱 [putty.org](https://www.putty.org) → **putty-64bit-…-installer.msi** 다운로드 → 기본 설치 (PuTTYgen, Pageant 포함됨 — D0-4에서 사용)

## Step 2. 세션 저장

🖱 PuTTY 실행 → **Host Name**: `<서버주소>` / **Port**: `<포트>` / Connection type: **SSH**
🖱 좌측 **Connection** → *Seconds between keepalives*: **30** (장시간 빌드 중 끊김 방지)
🖱 좌측 **Connection → Data** → *Auto-login username*: `stuNN` (본인 계정)
🖱 다시 **Session** → Saved Sessions에 `aosp-build` 입력 → **Save**

✅ **예상 결과:** 목록에 `aosp-build`가 생기고, 더블클릭만으로 접속 창이 뜸

## Step 3. 최초 접속 및 호스트 키 수락

🖱 `aosp-build` 더블클릭 → 보안 경고(호스트 키) → **Accept** → 비밀번호 입력(임시 — D0-4에서 키 인증으로 전환)

```bash
whoami && hostname
tmux -V
```

✅ **예상 결과:** 본인 계정명 + 서버 호스트명, `tmux 3.x` 버전 출력

## Step 4. 한글 깨짐 방지 (권장)

🖱 저장 세션 로드 → **Window → Translation** → Remote character set: **UTF-8** → Session에서 다시 Save

# 🏁 Pass 판정 체크리스트

- [ ] 저장 세션 더블클릭 → 비밀번호만으로 접속
- [ ] keepalive 30초 설정 저장됨
- [ ] `tmux -V` 정상 출력

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| Connection timed out | 사내망/VPN 미접속, 방화벽 | VPN 연결 후 재시도, 포트 확인 |
| Access denied | 계정/비밀번호 오타 | 운영측 안내 메일 재확인 |
| 접속 후 한글이 □□로 깨짐 | 문자셋 불일치 | Step 4의 UTF-8 설정 |

# 🚗 현업 활용 포인트

💡 세션 저장 + keepalive + UTF-8은 원격 개발의 기본 3종 세트입니다. 다음 실습(D0-4)에서 **비밀번호 인증을 키 인증으로 교체**하면 보안과 편의가 동시에 잡힙니다.

---
*실습 D0-3 (3/36) · 다음: **D0-4 SSH 키 페어 생성·등록***
