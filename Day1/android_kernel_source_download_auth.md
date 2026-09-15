# Android Kernel Source 다운로드 (인증 방식)

Android Common Kernel `common-android16-6.12` branch를 Google 계정 인증 후 다운로드하는 절차이다.

익명 접속(`/a/` 없는 URL)은 IP당 quota 제한이 있어 실습실처럼 동일 네트워크에서 다수가 동시에 sync하면 차단될 수 있다. 인증 방식은 계정별로 quota가 적용되므로 이 문제를 피할 수 있다.

---

## 사전 준비

- Google 계정
- `git`, `repo` 설치 완료
- Python 3

```bash
sudo apt update
sudo apt install -y git python3 curl
mkdir -p ~/bin
curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
chmod a+x ~/bin/repo
export PATH=~/bin:$PATH
```

---

## 1. git-cookies 발급

브라우저에서 아래 주소에 접속한다. Google 계정 로그인이 필요하다.

```
https://android.googlesource.com/new-password
```

로그인하면 `Configure Git` 이라는 제목으로 shell script가 표시된다. 해당 script 전체를 복사한다.

script 형태는 다음과 같다.

```bash
touch ~/.gitcookies
chmod 0600 ~/.gitcookies

git config --global http.cookiefile ~/.gitcookies

tr , \\t <<\__END__ >>~/.gitcookies
android.googlesource.com,FALSE,/,TRUE,2147483647,o,git-<계정>.gmail.com=1//<token>
android-review.googlesource.com,FALSE,/,TRUE,2147483647,o,git-<계정>.gmail.com=1//<token>
__END__
```

> `<계정>`, `<token>` 부분은 페이지에서 실제 값으로 채워져 표시된다. 그대로 복사해서 사용한다.

---

## 2. 터미널에서 script 실행

복사한 script를 터미널에 붙여넣고 실행한다.

실행 후 다음 항목을 확인한다.

```bash
cat ~/.gitcookies
git config --global --get http.cookiefile
```

- `~/.gitcookies` 파일에 `android.googlesource.com` 항목이 있어야 한다.
- `http.cookiefile` 값이 `~/.gitcookies` 경로여야 한다.

---

## 3. 인증 확인

인증이 정상 동작하는지 `/a/` URL로 확인한다.

```bash
git ls-remote https://android.googlesource.com/a/kernel/manifest | grep android16
```

branch 목록이 출력되면 인증 성공이다. `authentication` 관련 오류가 나오면 1~2단계를 다시 확인한다.

---

## 4. repo init

이전에 실패한 `.repo` 디렉토리가 있다면 삭제 후 진행한다.

```bash
mkdir -p ~/android-kernel
cd ~/android-kernel
rm -rf .repo

repo init \
    -u https://android.googlesource.com/a/kernel/manifest \
    -b common-android16-6.12 \
    --depth=1
```

> 익명 URL과 다르게 `/a/` 가 포함된 URL을 사용한다.

---

## 5. repo sync

```bash
repo sync -c -j$(nproc) --no-tags --no-clone-bundle
```

옵션 설명

| 옵션 | 설명 |
|------|------|
| `-c` | 현재 branch만 fetch (용량 절감) |
| `-j$(nproc)` | CPU 코어 수만큼 병렬 fetch |
| `--no-tags` | tag 제외 |
| `--no-clone-bundle` | bundle 다운로드 생략 (네트워크 문제 시 유리) |

---

## 6. 결과 확인

```bash
cd ~/android-kernel
ls
ls common
```

`common/` 디렉토리 아래에 kernel source가 있으면 완료이다.

---

## 문제 해결

### `git requires authentication, but repo cannot perform interactive authentication`

- `~/.gitcookies` 가 없거나 `http.cookiefile` 설정이 안 된 상태이다.
- 1~2단계를 다시 실행한다.

### `fatal: cannot obtain manifest`

- `.repo` 디렉토리를 삭제하고 `repo init` 을 다시 실행한다.
- branch 이름이 정확한지 3단계로 확인한다.

### sync 중 `429 Too Many Requests`

- 계정 quota 초과. 잠시 후 재시도한다.
- 실습실에서 다수가 동시에 받는 경우 시간차를 두거나, 강사 PC에서 받은 소스를 tarball로 배포한다.

### token 재발급

- 기존 token이 만료되거나 유출된 경우 `https://android.googlesource.com/new-password` 에서 새로 발급받고 `~/.gitcookies` 의 해당 줄을 교체한다.
