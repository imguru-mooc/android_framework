# 실습 1. 빌드 서버 접속 — PuTTYgen · PuTTY · WinSCP (Ed25519 공개키)

> **소요시간:** 35분 · **난이도:** ★☆☆
> **환경:** Windows 11 (PuTTY, PuTTYgen, WinSCP) + Ubuntu 24 빌드 서버

## 목표

- PuTTYgen 대화창에서 **EdDSA / Ed25519** 키 쌍을 만든다.
- 빌드 서버에 공개키를 등록해 **ID·비밀번호 입력 없이** PuTTY로 접속한다.
- WinSCP를 같은 키로 설정해 파일을 드래그 앤 드롭으로 주고받는다.

## 원리

| 위치 | 보관 항목 | 파일 |
|---|---|---|
| Windows PC | 개인키 (private key) | `C:\Users\<이름>\.ssh\build-server.ppk` |
| Ubuntu 서버 | 공개키 (public key) | `~/.ssh/authorized_keys` 의 한 줄 |

개인키는 절대 서버로 보내지 않는다. 서버에는 공개키 한 줄만 올라간다.

---

## Step 1. PuTTY · WinSCP 다운로드 및 설치

### PuTTY (PuTTYgen 포함)

| 순서 | 내용 |
|---|---|
| 1 | 브라우저에서 https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html 접속 |
| 2 | **Package files → MSI ('Windows Installer')** → `putty-64bit-x.xx-installer.msi` 다운로드 |
| 3 | 다운로드한 MSI 실행 → **Next** → 설치 경로 기본값 → **Install** → **Finish** |
| 4 | 시작 메뉴에 **PuTTY**, **PuTTYgen**, **Pageant** 가 생성되었는지 확인 |

> 64-bit x86 버전을 선택한다. Zip 버전을 받았다면 `putty.exe`, `puttygen.exe` 를 작업 폴더에 풀어 써도 된다.

### WinSCP

| 순서 | 내용 |
|---|---|
| 1 | https://winscp.net/eng/download.php 접속 → **Download WinSCP** (`WinSCP-x.x.x-Setup.exe`) |
| 2 | 설치 파일 실행 → **Install for all users** → 라이선스 **Accept** |
| 3 | Setup type **Typical installation** → **Install** |
| 4 | 첫 실행 시 인터페이스 선택 → **Commander** (좌: Windows / 우: 서버 두 창 구조) → **OK** |

---

## Step 2. PuTTYgen으로 Ed25519 키 생성

| 순서 | 대화창 설정 |
|---|---|
| 1 | 시작 메뉴 → **PuTTYgen** 실행 |
| 2 | 하단 **Parameters** → **Type of key to generate** → **EdDSA** 선택 |
| 3 | 바로 아래 **Curve to use for generating this key** → **Ed25519 (255 bits)** 선택 |
| 4 | **Generate** 클릭 → 빈 영역 위에서 마우스를 움직여 키 생성 |
| 5 | **Key comment** : `홍길동@build-server` 처럼 식별 가능한 이름 입력 |
| 6 | **Key passphrase / Confirm passphrase** : 비워 둔다 (교육용, 접속 시 암호 입력 없음) |
| 7 | **Save private key** → `C:\Users\<이름>\.ssh\build-server.ppk` 저장 (passphrase 없음 경고는 **예**) |
| 8 | 상단 **Public key for pasting into OpenSSH authorized_keys file** 박스의 **한 줄 전체**를 복사해 메모장에 붙여 둔다 |

복사한 한 줄은 다음 형태다.

```text
ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAI... 홍길동@build-server
```

> ⚠ **Save public key** 버튼으로 저장한 파일은 PuTTY 전용 형식이라 OpenSSH 서버에 그대로 쓸 수 없다. 반드시 8번의 한 줄 텍스트를 사용한다.

---

## Step 3. Ubuntu 빌드 서버에 공개키 등록 (최초 1회)

최초 1회는 강사가 알려준 비밀번호로 PuTTY 접속한 뒤 진행한다.

```bash
mkdir -p ~/.ssh
chmod 700 ~/.ssh
nano ~/.ssh/authorized_keys      # PuTTYgen에서 복사한 ssh-ed25519 한 줄 붙여넣기 → Ctrl+O, Enter, Ctrl+X
chmod 600 ~/.ssh/authorized_keys
cat ~/.ssh/authorized_keys        # 한 줄이 잘려 있지 않은지 확인
```

sshd 설정 확인 (관리자):

```bash
sudo grep -E "^(PubkeyAuthentication|PasswordAuthentication)" /etc/ssh/sshd_config
# PubkeyAuthentication yes 이어야 함 (기본값)
sudo systemctl restart ssh
```

---

## Step 4. PuTTY 세션 설정 — ID 입력 없이 자동 로그인

| 순서 | 대화창 설정 |
|---|---|
| 1 | **PuTTY** 실행 → 좌측 트리 **Session** |
| 2 | **Host Name (or IP address)** : 빌드 서버 IP / **Port** : `22` / **Connection type** : SSH |
| 3 | 좌측 **Connection → Data** → **Auto-login username** : 서버 계정명 (예: `worker`) |
| 4 | 좌측 **Connection → SSH → Auth → Credentials** → **Private key file for authentication** → **Browse…** → `build-server.ppk` 선택 |
| 5 | 좌측 **Window → Translation** → **Remote character set** : `UTF-8` (한글 깨짐 방지) |
| 6 | 좌측 **Session** 으로 돌아가 **Saved Sessions** 에 `build-server` 입력 → **Save** |
| 7 | **Open** → 최초 접속 시 호스트 키 경고 → **Accept** |

정상이면 ID·비밀번호 프롬프트 없이 바로 쉘이 뜬다.

```text
Using username "worker".
Authenticating with public key "홍길동@build-server"
worker@build-server:~$
```

이후에는 PuTTY 실행 → `build-server` 더블클릭만으로 접속된다.

---

## Step 5. WinSCP 설정

| 순서 | 대화창 설정 |
|---|---|
| 1 | **WinSCP** 실행 → 로그인 창에서 **New Session** |
| 2 | **File protocol** : SFTP / **Host name** : 빌드 서버 IP / **Port number** : `22` |
| 3 | **User name** : 서버 계정명 / **Password** : 비워 둠 |
| 4 | **Advanced…** → 좌측 **SSH → Authentication** → **Private key file** → `build-server.ppk` 선택 → **OK** |
| 5 | (선택) **Advanced… → Environment → Directories** → **Remote directory** : `/home/<계정>/aosp` |
| 6 | **Save** → Site name `build-server` → **OK** |
| 7 | **Login** → 좌측 Windows / 우측 서버 디렉토리가 표시되면 성공 |

> 💡 PuTTY 세션을 먼저 저장했다면 WinSCP 로그인 창 **Tools → Import Sites** 로 PuTTY 세션(키 설정 포함)을 그대로 가져올 수 있다.

---

## 확인 포인트

- [ ] PuTTY `build-server` 더블클릭 → ID·비밀번호 없이 프롬프트 표시
- [ ] WinSCP 로그인 → `~/aosp` 폴더 탐색, 파일 드래그 앤 드롭 전송 성공
- [ ] 서버 `~/.ssh/authorized_keys` 권한 `600`, `~/.ssh` 권한 `700`

## 트러블슈팅

| 증상 | 원인 / 해결 |
|---|---|
| `Server refused our key` | `authorized_keys` 한 줄이 잘렸거나 권한 오류 → 다시 붙여넣고 `chmod 600` |
| 비밀번호를 계속 물어봄 | Auth → Credentials 에 `.ppk` 미지정, 또는 PuTTYgen에서 만든 키와 다른 키 |
| `Auto-login username` 이 안 보임 | Connection → Data 항목 (SSH 하위가 아님) |
| 한글 파일명 깨짐 | PuTTY Translation UTF-8, WinSCP는 기본 UTF-8 |
