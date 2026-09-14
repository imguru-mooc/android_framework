# Day 1 — 환경 설정 · Android Architecture · Boot · Binder / AIDL

**과정:** Android Framework 16 심화 교육 (4일 / 32시간)
**환경:** Windows 11 (Android Studio, Emulator API 36) + Ubuntu 24 빌드 서버 (AOSP `aosp_car_x86_64`, GKI 6.12)
**범위:** 환경 설정 + Chapter 0~3 (Architecture, Binder, AIDL, IPC Service) + Kernel Module 빌드 데모
**총 소요시간:** 8시간 (환경 설정 실습 1h 40m / 강의 2h 10m / 실습 3h 35m / 데모·Worksheet 35m)
**실습 번호:** Part 0 환경 설정부터 실습 1 ~ 13 으로 순번 부여

---

## 학습 목표

1. Windows PC에서 빌드 서버(Ubuntu)에 SSH 공개키로 접속하고 파일을 주고받을 수 있다.
2. Android Studio·Emulator·adb 실습 환경을 스스로 구성하고, 빌드 서버에서 AOSP를 빌드해 결과물을 가져올 수 있다.
3. Android가 Linux 위에서 어떻게 부팅되고 어떤 Process 구조로 동작하는지 설명할 수 있다.
4. Binder가 왜 Android의 표준 IPC인지, Driver·Proxy·Stub 구조를 이해한다.
5. AIDL로 별도 Process의 Service를 직접 구현하고, 동기/비동기(oneway)·Callback·DeathRecipient 패턴을 적용할 수 있다.
6. Messenger가 Binder 위에서 어떻게 동작하는지, Binder Transaction 크기 한계와 FD 기반 대안을 이해한다.
7. Android 16 GKI 커널에서 `ddk_module`로 커널 모듈이 빌드·로드되는 흐름을 안다.

---

## Part 0. 환경 설정 실습 (총 100분)

| # | 실습명 | 난이도 | 소요시간 |
|---|---|---|---|
| 1 | 빌드 서버 접속 — PuTTY·WinSCP 설치, PuTTYgen Ed25519, 자동 로그인 | ★☆☆ | 35분 |
| 2 | Android Studio · Emulator 설치 | ★☆☆ | 20분 |
| 3 | adb 접속 방법 | ★☆☆ | 15분 |
| 4 | 빌드 서버에서 Android 빌드 | ★★☆ | 30분 |

### 실습 1. 빌드 서버 접속 — PuTTY · WinSCP 설치 / PuTTYgen Ed25519 / 자동 로그인 — 35분

#### ① PuTTY · WinSCP 다운로드 및 설치

| 프로그램 | 다운로드 | 설치 |
|---|---|---|
| PuTTY (PuTTYgen 포함) | https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html → MSI 64-bit | MSI 실행 → Next → Install → 시작 메뉴에 PuTTY / PuTTYgen 확인 |
| WinSCP | https://winscp.net/eng/download.php → Setup.exe | Install for all users → Typical → 첫 실행 시 Commander 인터페이스 |

#### ② PuTTYgen으로 Ed25519 키 생성 (Windows 대화창)

| 순서 | 대화창 설정 |
|---|---|
| 1 | 시작 메뉴 → **PuTTYgen** 실행 |
| 2 | 하단 **Parameters** → **Type of key to generate** → **EdDSA** 선택 |
| 3 | 바로 아래 **Curve to use for generating this key** → **Ed25519 (255 bits)** 선택 |
| 4 | **Generate** 클릭 → 빈 영역 위에서 마우스를 움직여 키 생성 |
| 5 | **Key comment**: `홍길동@build-server` 처럼 식별 가능한 이름 입력 |
| 6 | **Key passphrase / Confirm passphrase**: 비워 두면 접속 시 암호 입력 없음 (교육용) |
| 7 | **Save private key** → `C:\Users\<이름>\.ssh\build-server.ppk` 저장 (passphrase 없음 경고는 **예**) |
| 8 | 상단 **Public key for pasting into OpenSSH authorized_keys file** 박스의 한 줄 전체(`ssh-ed25519 AAAA… 주석`)를 복사해 메모장에 붙여 두기 |

> `Save public key` 버튼으로 저장한 파일은 PuTTY 전용 형식이므로 서버에는 8번의 한 줄 텍스트를 등록한다.

#### ③ Ubuntu 빌드 서버에 공개키 등록 (최초 1회, 비밀번호 로그인으로 진행)

```bash
mkdir -p ~/.ssh && chmod 700 ~/.ssh
nano ~/.ssh/authorized_keys      # PuTTYgen에서 복사한 ssh-ed25519 한 줄 붙여넣기 → 저장
chmod 600 ~/.ssh/authorized_keys
```

- sshd 확인: `/etc/ssh/sshd_config`에 `PubkeyAuthentication yes` → `sudo systemctl restart ssh`

#### ④ PuTTY 세션 설정 — ID 입력 없이 자동 로그인

| 순서 | 대화창 설정 |
|---|---|
| 1 | **PuTTY** 실행 → 좌측 **Session** |
| 2 | **Host Name (or IP address)**: 빌드 서버 IP, **Port**: `22`, **Connection type**: SSH |
| 3 | 좌측 **Connection → Data** → **Auto-login username**: 서버 계정명 (예: `worker`) ← 이 설정으로 ID 입력 생략 |
| 4 | 좌측 **Connection → SSH → Auth → Credentials** → **Private key file for authentication** → **Browse** → `build-server.ppk` 선택 |
| 5 | (선택) **Window → Translation** → Remote character set: `UTF-8` (한글 깨짐 방지) |
| 6 | 좌측 **Session**으로 돌아가 **Saved Sessions**에 `build-server` 입력 → **Save** |
| 7 | **Open** → 최초 접속 시 호스트 키 경고 **Accept** → ID·비밀번호 입력 없이 프롬프트 표시 |

이후에는 PuTTY 실행 → `build-server` 더블클릭만으로 접속된다.

#### ⑤ WinSCP 설정

| 순서 | 대화창 설정 |
|---|---|
| 1 | **WinSCP** 실행 → **New Session** (로그인 창) |
| 2 | **File protocol**: SFTP / **Host name**: 빌드 서버 IP / **Port number**: `22` |
| 3 | **User name**: 서버 계정명 / **Password**: 비워 둠 |
| 4 | **Advanced…** → 좌측 **SSH → Authentication** → **Private key file** → `build-server.ppk` 선택 → **OK** |
| 5 | (선택) **Environment → Directories** → Remote directory: `/home/<계정>/aosp` |
| 6 | **Save** → Site name `build-server`, **Save password** 체크 불필요 → **OK** |
| 7 | **Login** → 좌측 Windows / 우측 서버 디렉토리 표시 확인 |

> PuTTY 세션을 먼저 저장했다면 WinSCP 로그인 창 **Tools → Import Sites**로 PuTTY 세션(키 포함)을 그대로 가져올 수도 있다.

**확인 포인트:** PuTTY 더블클릭 접속 시 ID·비밀번호 프롬프트 없음, WinSCP로 `~/aosp` 탐색 및 드래그 앤 드롭 전송 가능
**핵심 원칙:** 개인키(`.ppk`)는 Windows에만 보관, 서버 `authorized_keys`에는 공개키 한 줄만 등록. Ed25519는 RSA 4096보다 키가 짧고 빠르며 현재 권장 방식

### 실습 2. Android Studio 및 Emulator 설치 — 20분

| 단계 | 내용 |
|---|---|
| Android Studio 설치 | 최신 안정 버전 설치, JDK 번들 사용 |
| AVD 생성 | Device Manager → Automotive 카테고리 → 기본 선택 **API 35** Google APIs 이미지 → `Automotive_1408p_landscape` (Automotive 는 API 36 이미지 미제공) |
| Emulator 실행 확인 | AVD 부팅 → Car Launcher 화면 확인 |
| 환경 변수 | `%LOCALAPPDATA%\Android\Sdk\platform-tools` 를 PATH에 추가 |

**확인 포인트:** cmd에서 `adb version` 출력, AVD 정상 부팅
**주의:** Google APIs 이미지여야 `adb root`가 가능 (Google Play 이미지 불가)

### 실습 3. adb 접속 방법 — 15분

| 명령 | 용도 |
|---|---|
| `adb devices` | 연결된 Emulator/디바이스 확인 (`emulator-5554 device`) |
| `adb root` | root 권한으로 adbd 재시작 (Google APIs / userdebug) |
| `adb shell` | Emulator 쉘 진입, `exit`로 복귀 |
| `adb push <file> /data` / `adb pull` | 파일 전송, `chmod 755` 후 실행 |
| `adb remount` | `/system` 쓰기 가능하게 (최초 1회 `adb reboot` 필요) |
| `adb logcat -s <TAG>` | 태그 필터 로그 |
| `adb shell dumpsys <service>` | System Service 상태 덤프 |
| `adb kill-server` / `adb start-server` | 연결 이상 시 재시작 |

**확인 포인트:** `adb root` → `adb shell id` → `uid=0(root)` 출력

### 실습 4. 빌드 서버에서 Android 빌드 — 30분 (전원 실습)

| 단계 | 내용 |
|---|---|
| 소스 위치 확인 | `cd ~/aosp` (사전 `repo sync` 완료 상태), `ls`로 `build/`, `frameworks/`, `out/` 확인 |
| 빌드 환경 설정 | `source build/envsetup.sh` |
| 타겟 선택 | `lunch sdk_car_x86_64-aosp_current-userdebug` (Android 16 3-파트 형식: product-release-variant) |
| 전체 빌드 | `m -j$(nproc)` (각 계정에 사전 빌드 완료 → 증분 빌드로 수 분 내 완료) |
| Emulator 이미지 생성 | `m emu_img_zip` → `out/target/product/emulator_car64_x86_64/sdk-repo-linux-system-images.zip` |
| Windows로 가져오기 | WinSCP로 `C:\aosp16`에 다운로드 후 압축 해제 |
| 커스텀 이미지로 Emulator 실행 | `run_custom_car_emulator.bat` (`-sysdir C:\aosp16\x86_64 -avd Automotive_1408p_landscape -writable-system -no-snapshot-load -qemu -append "androidboot.selinux=permissive"`) |
| 모듈 단위 빌드 | 모듈 디렉토리에서 `mm -j$(nproc)` — 이후 실습에서 주로 사용 |
| 결과물 배포 흐름 | `mm` 빌드 → `$OUT/system/bin/<binary>` → WinSCP 다운로드 → `adb push /data` → `chmod 755` → 실행 |

**확인 포인트:** `echo $OUT` 경로 확인, `sdk-repo-linux-system-images.zip` 존재, 커스텀 이미지 Emulator 부팅
**핵심:** `lunch`는 터미널 세션마다 다시 실행, 전체 빌드는 사전 빌드된 트리에서 증분으로 수행, 이후 실습은 모듈 빌드(`mm`) 위주

---

## Part 1. 강의 (총 130분)

### Chapter 0. Android Architecture & Boot — 40분

| 주제 | 핵심 내용 |
|---|---|
| Android Architecture | Linux Kernel → HAL → Native / ART → Framework → App 계층 구조 |
| Linux vs Android | Bionic, init, Binder, ashmem/dmabuf, SELinux 등 Android 고유 요소 |
| Boot Sequence | Bootloader → Kernel → init (PID 1) → init.rc → Zygote → system_server |
| init & init.rc | Service 정의 문법, `init.zygote64_32.rc`, class/trigger |
| Zygote | Preload, `fork()` 기반 App Process 생성, 모든 App의 부모 |
| system_server | 주요 System Service가 실행되는 핵심 Process (AMS, WMS, PMS…) |
| ServiceManager | Service 등록·조회, `service list`, `dumpsys` |

### Chapter 1. Binder 구조 — 45분

| 주제 | 핵심 내용 |
|---|---|
| Binder의 목적 | Process 간 통신(IPC) — 왜 Socket/Pipe가 아닌가 |
| Binder Driver | `/dev/binder`, binderfs, 1-copy 전달, Transaction Buffer (1MB) |
| Proxy / Stub | Client 측 Proxy(Bp) ↔ Server 측 Stub(Bn), `transact()` / `onTransact()` |
| Binder Thread Pool | 기본 16 Thread, Binder Thread에서 콜백 실행 → UI 접근 주의 |
| 보안 | `getCallingUid()/getCallingPid()`, `clearCallingIdentity()` 패턴 |
| Transaction 한계 | TransactionTooLargeException, 대용량은 FD/ashmem으로 |
| 관찰 도구 | `binderfs/binder_logs`, ftrace `binder_transaction`, `service call` |

### Chapter 2. AIDL & IPC 패턴 — 30분

| 주제 | 핵심 내용 |
|---|---|
| AIDL 기본 | `.aidl` 정의 → Stub/Proxy 자동 생성, `buildFeatures { aidl = true }` |
| 지원 타입 | 기본형, String, List, Parcelable, `in/out/inout` |
| 동기 vs oneway | 동기 호출은 reply 대기, `oneway`는 즉시 반환 |
| Callback 패턴 | Client → Service 역방향 인터페이스, `RemoteCallbackList` |
| DeathRecipient | Service Process 사망 감지, `linkToDeath()`, `BIND_AUTO_CREATE` 재바인딩 |
| `android:process` | 별도 Process로 분리해야 실제 IPC 발생 |

### Chapter 3. Framework 내장 IPC — 15분

| 주제 | 핵심 내용 |
|---|---|
| Messenger | AIDL 없이 Handler 기반 IPC, 단일 Thread 순차 처리, `replyTo` |
| Content Provider | `ContentResolver` 내부 Binder, CursorWindow(공유 메모리), ContentObserver |
| 선택 기준 | AIDL vs Messenger vs Content Provider 비교 |

---

## Part 2. 실습 (총 215분)

| # | 실습명 | 난이도 | 소요시간 | 챕터 |
|---|---|---|---|---|
| 5 | 시스템 탐험가 — adb로 Android 내부 들여다보기 | ★☆☆ | 20분 | Ch 0 |
| 6 | Binder 탐정 — System Service 추적 + 직접 만든 Binder Server/Client trace | ★★☆ | 30분 | Ch 1 |
| 7 | AIDL 계산기 — 기본 IPC Service 구현 | ★★☆ | 45분 | Ch 2 |
| 8 | Binder 생존 게임 — DeathRecipient & 복구 | ★★☆ | 30분 | Ch 2 |
| 9 | 비동기 주식 시세 Service — oneway + Callback | ★★★ | 40분 | Ch 2 |
| 10 | Messenger 채팅 앱 — 경량 IPC | ★★☆ | 30분 | Ch 3 |
| 11 | TransactionTooLargeException 재현 & 해결 | ★★☆ | 20분 | Ch 1 |
| 12 | Content Provider 메모장 — 멀티프로세스 데이터 공유 (선택/과제) | ★★★ | 45분 | Ch 3 |

> 실습 12는 시간 여유 시 진행하고, 그렇지 않으면 예제 소스를 배포해 자율 과제로 전환한다.

### 실습 5. 시스템 탐험가 — 20분
- `adb root` / `adb shell` → `ps -A -o pid,ppid,name`으로 init·Zygote·system_server 확인
- `service list | wc -l`, `dumpsys activity`, `binderfs/binder_logs/stats`
- `init.zygote64_32.rc` 읽기, `dmesg | grep binder`
- **확인 포인트:** Zygote PPID = 1, system_server PPID = Zygote PID, 모든 App PPID = Zygote
- **결과물:** PID 기록, System Service 총 개수, Binder Transaction 상위 Service 추정

### 실습 6. Binder 탐정 — 30분
- `service call clipboard 1`, `dumpsys display`, `dumpsys battery`
- ftrace: `events/binder/binder_transaction/enable` → `trace_pipe` 실시간 관찰
- Settings 앱 PID의 `binder_logs/proc/<PID>`에서 Binder Thread 수 확인
- `hello_server`(BBinder 직접 구현, addService) 를 `mm` 으로 빌드해 띄우고 `hello_client` 가 transact → ftrace `dest_proc` 필터로 요청/응답 3줄 확인
- **결과물:** Settings 앱 실행 시 발생하는 Transaction 수, 호출된 Service 추정

### 실습 7. AIDL 계산기 — 45분
- 프로젝트 `BinderLab`, `ICalculatorService.aidl` (add/subtract/multiply/divide/getCallerInfo/getHistory)
- `CalculatorService` (Stub 구현, `clearCallingIdentity` 패턴), `android:process=":calc_remote"`
- `MainActivity`에서 `bindService` → `Stub.asInterface()`
- **확인 포인트:** `getCallerInfo()` → "Same process: NO", `ps | grep binderlab` 2개 Process
- **변형:** `android:process` 제거 후 "Same process: YES" 비교

### 실습 8. Binder 생존 게임 — 30분
- 실습 7 프로젝트 연속, `kill -9 <calc_remote PID>`
- DeathRecipient 콜백 → "Service DIED! Rebinding" → 자동 재바인딩
- 반복 kill 스트레스 테스트 (5회)
- **핵심:** DeathRecipient는 Binder Thread에서 호출 → `runOnUiThread()` 필수, `onServiceDisconnected`보다 먼저 호출

### 실습 9. 비동기 주식 시세 Service — 40분
- 프로젝트 `StockLab`, `IStockCallback.aidl`(oneway) + `IStockService.aidl`
- `getPrice()` 동기 2초 블로킹 vs `subscribe()` oneway 0ms 즉시 반환
- `RemoteCallbackList`로 Client 사망 시 자동 정리
- **확인 포인트:** Logcat `StockService`/`StockClient` PID가 서로 다름, 콜백에서 `runOnUiThread`

### 실습 10. Messenger 채팅 앱 — 30분
- 프로젝트 `MessengerChat`, AIDL 파일 없음 (`buildFeatures.aidl` 불필요)
- `ChatConstants` / `ChatService`(IncomingHandler) / `MainActivity`, `android:process=":chat_remote"`
- 테스트: Echo·브로드캐스트, 서버 시간 요청, 연속 5개 메시지 순서 보장, 서비스 kill
- **핵심:** Handler 단일 Thread 순차 처리 (AIDL은 멀티 Thread)

### 실습 11. TransactionTooLargeException 재현 & 해결 — 20분
- 프로젝트 `BinderLimitLab`, `IDataService.aidl` (byte[] + ParcelFileDescriptor)
- 10KB → 100KB → 1MB 점진 증가로 한계 탐색
- Pipe/FD 전달 방식과 속도 비교
- **핵심:** 대용량 데이터는 항상 Pipe/FD 사용

### 실습 12. Content Provider 메모장 — 45분 (선택/과제)
- 프로젝트 `MemoProvider`, `MemoContract` (URI `content://com.example.memoprovider.memo/memos`)
- SQLite 기반 Provider CRUD, ContentObserver 실시간 알림
- `adb shell content query/insert` 로 외부에서 직접 접근
- **핵심:** ContentResolver 내부는 Binder, 대용량 결과는 CursorWindow(FD 공유 메모리)로 전달

---

## Part 3. 데모 & 정리 (총 35분)

### 실습 13 (데모). Kernel Module 빌드 — 15분 (강사 시연)
- Android 16 GKI (Linux 6.12) 커널 다운로드·빌드 흐름 소개 (`repo init … common-android16-6.12`, `tools/bazel run //common:kernel_x86_64_dist`)
- 빌드된 `bzImage`·`.ko`를 `prebuilts/qemu-kernel/x86_64/6.12`에 교체 후 AOSP 리빌드 → 커스텀 Emulator 이미지
- `kernel_module()` deprecated → `ddk_module()` (Kleaf DDK)
- `hello_android.c` + `BUILD.bazel` 2개 파일만으로 `.ko` 빌드 (Kbuild/Makefile 없음)
- `adb push` → `insmod` → `cat /proc/hello_android` → `rmmod`
- **핵심:** `kernel_build`는 `virtual_device_x86_64`, `vermagic` 일치 필수, `--check_visibility=false`

### Day 1 Worksheet — 20분
- 4지선다 객관식 12~15문제 (쉬움 40% / 보통 50% / 약간 어려움 10%)
- 별도 정답·해설 페이지 제공

**출제 키워드**
```text
SSH 공개키 / 개인키 (Ed25519)
PuTTYgen / PuTTY / WinSCP
adb root / adb push / adb shell
envsetup.sh / lunch / m / mm
Android Architecture
Linux vs Android
Boot Sequence
init / init.rc
Zygote
system_server
ServiceManager
Binder
Binder Driver / Proxy / Stub
Binder Thread
AIDL
oneway
Callback / RemoteCallbackList
DeathRecipient
android:process
Messenger
TransactionTooLargeException
ddk_module
```

---

## 준비물 체크리스트

**강사 / 사전 준비**
- [ ] Ubuntu 24 빌드 서버: `~/aosp` AOSP 소스 `repo sync` + `lunch sdk_car_x86_64-aosp_current-userdebug` 전체 빌드 완료
- [ ] `~/android-kernel` GKI 6.12 커널 빌드 완료 (데모용)
- [ ] 수강생별 서버 계정 생성, sshd `PubkeyAuthentication yes`
- [ ] 수강생 공개키 사전 수집 시 `authorized_keys` 미리 등록 (선택)
- [ ] `sdk-repo-linux-system-images.zip`, `run_custom_car_emulator.bat` 사전 배포
- [ ] 실습 5~12 예제 소스 (`day1_lab_examples.md`), `winscp_rsa_ssh_guide.md`, `android16_kernel_module_guide.md` 배포
- [ ] Day 1 Worksheet 인쇄본 + 정답지

**수강생 PC (Windows 11)**
- [ ] PuTTY (PuTTYgen 포함), WinSCP 설치
- [ ] Android Studio 최신 버전, Automotive API 35 Google APIs x86_64 System Image
- [ ] Automotive AVD (`Automotive_1408p_landscape`) 생성
- [ ] `platform-tools` PATH 등록, `adb devices` 동작 확인
