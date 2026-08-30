# [D0-4] SSH 공개키/개인키 페어 생성·등록 — PuTTY·WinSCP 공용

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★☆☆☆ | Day 0 ★전원 필수 | PuTTY 접속 가능(D0-3) |

> 🎯 **실습 목표** — PuTTYgen으로 **Ed25519 키 페어**를 만들고 서버에 공개키를 등록해, PuTTY와 WinSCP 모두 **키+passphrase 인증**으로 전환한다. WinSCP(D0-5)의 자동 업로드 워크플로의 전제 조건이다.

## Step 1. 키 페어 생성 (PuTTYgen)

🖱 시작 메뉴 → **PuTTYgen** 실행 → Type of key: **EdDSA (Ed25519)** 선택 → **Generate** → 진행바 위에서 마우스를 휘저어 엔트로피 생성

🖱 **Key passphrase**: 기억할 수 있는 문구 입력(2회) — ⚠️ 빈 passphrase 금지(키 파일 유출 = 계정 유출)

🖱 **Save private key** → `C:\Users\<이름>\.ssh\aosp_ed25519.ppk` 로 저장
🖱 상단의 **"Public key for pasting into OpenSSH authorized_keys file"** 텍스트 전체를 복사(한 줄!)

✅ **예상 결과:** `.ppk` 파일 저장 + 클립보드에 `ssh-ed25519 AAAA... comment` 한 줄

## Step 2. 서버에 공개키 등록

PuTTY(비밀번호 인증)로 접속한 뒤:

```bash
mkdir -p ~/.ssh && chmod 700 ~/.ssh
echo '붙여넣은_공개키_한_줄' >> ~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys
tail -1 ~/.ssh/authorized_keys
```

⚠️ 공개키는 **반드시 한 줄** — 붙여넣기 시 줄바꿈이 섞이면 인증이 조용히 실패합니다. `tail -1` 출력이 `ssh-ed25519`로 시작하는 한 줄인지 확인.

## Step 3. PuTTY를 키 인증으로 전환

🖱 PuTTY → 세션 `aosp-build` Load → **Connection → SSH → Auth → Credentials** → *Private key file*: `aosp_ed25519.ppk` 지정 → Session에서 **Save** → 접속

✅ **예상 결과:** 비밀번호 대신 **"Passphrase for key"** 를 묻고, 입력 후 로그인 성공

## Step 4. (선택) Pageant로 passphrase 1회 입력

🖱 **Pageant** 실행(트레이) → Add Key → `.ppk` 선택 → passphrase 1회 입력 → 이후 PuTTY/WinSCP 모두 자동 인증

# 🏁 Pass 판정 체크리스트

- [ ] Ed25519 `.ppk` 생성 + passphrase 설정
- [ ] 서버 `~/.ssh/authorized_keys`에 한 줄 등록 (700/600 권한)
- [ ] PuTTY 키 인증 접속 성공 (비밀번호 미사용)

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| Server refused our key | 공개키 줄바꿈 섞임 / 권한 잘못 | Step 2 재수행: 한 줄 확인 + `chmod 700/600` |
| 여전히 비밀번호를 물음 | 세션에 .ppk 미지정 / Save 안 함 | Step 3에서 Load→지정→**Save** 순서 확인 |
| passphrase 분실 | 복구 불가 | 키 재생성 후 재등록 (Step 1~2) |

# 🚗 현업 활용 포인트

💡 사내 빌드/배포 서버는 대부분 **비밀번호 인증 금지, 키 인증 필수**입니다. "개인키는 내 PC 밖으로 절대 안 나간다, 서버엔 공개키만"이라는 비대칭 구조를 오늘 손으로 익혀두면 CI 배포 키 관리도 같은 문법으로 읽힙니다.

---
*실습 D0-4 (4/36) · 다음: **D0-5 WinSCP 설정 + 북마크***
