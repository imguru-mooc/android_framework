# Day 2 — Native Binder Service · HAL · System Service

**과정:** Android Framework 16 심화 교육 (4일 / 32시간)
**환경:** Ubuntu 24 빌드 서버 (`~/aosp`, AOSP `sdk_car_x86_64`) + Windows (WinSCP, adb) + 커스텀 Emulator (userdebug, permissive)
**범위:** libbinder 수작업 Bn/Bp → AIDL cpp/ndk backend → UID 접근 제어 → init.rc · SELinux → AIDL HAL · VINTF → Java System Service
**총 소요시간:** 8시간 (환경·등록 실습 20m / 강의 2h 10m / 실습 5h 05m / 데모·Worksheet 35m)
**실습 번호:** 실습 1 ~ 13 (Day 1 과 동일하게 Part 0 부터 연속 번호)

---

## 학습 목표

1. `ProcessState` · `IServiceManager` · `IPCThreadState` · `BBinder` · `BpBinder` 로 Native Binder 의 계층을 설명할 수 있다.
2. `ILedService` / `BnLedService` / `BpLedService` 를 손으로 작성해 AIDL 이 생성하는 코드의 실체를 안다.
3. `Parcel` 로 인자·반환값·IBinder(Callback) 를 직렬화할 수 있다.
4. `aidl_interface` 로 cpp / ndk backend 라이브러리를 만들고 `binder::Status` / `ScopedAStatus` 로 예외를 전달할 수 있다.
5. `getCallingUid()` 로 접근 제어를 구현하고 `EX_SECURITY` 가 Java `SecurityException` 이 되는 경로를 안다.
6. `init.rc` 로 데몬을 등록·제어하고 `avc: denied` 로그를 읽어 필요한 SELinux 정책을 말할 수 있다.
7. AIDL HAL 의 규칙(패키지명, NDK backend, vendor, VINTF manifest) 을 이해하고 최소 HAL 을 만들 수 있다.
8. `SystemServer.java` 의 Service 등록 경로가 오늘 만든 C++ 서비스와 같은 `servicemanager` 에 도착함을 확인한다.

---

## Part 0. 환경·등록 실습 (총 20분)

| # | 실습명 | 난이도 | 소요시간 |
|---|---|---|---|
| 1 | Day 2 환경 점검 · 실습 모듈 `Android.bp` 일괄 등록 (Soong 재분석 1회로 끝내기) | ★☆☆ | 20분 |

---

## Part 1. 강의 (총 130분)

### Chapter 1. Native Binder — libbinder 계층 — 45분

| 주제 | 핵심 내용 |
|---|---|
| libbinder 구조 | `ProcessState`(Process 당 1, `/dev/binder` open·mmap·Thread Pool) · `IPCThreadState`(Thread 당 1, `transact` / `joinThreadPool`) |
| 객체 모델 | `IBinder` ← `BBinder`(로컬) / `BpBinder`(원격 handle) · `IInterface` ← `BnInterface<I>` / `BpInterface<I>` |
| 메타 인터페이스 | `DECLARE_META_INTERFACE` · `IMPLEMENT_META_INTERFACE(descriptor)` · `interface_cast<>` = `asInterface` |
| Parcel | `writeInt32 / writeString16 / writeStrongBinder / writeFileDescriptor`, 쓰기 순서 = 읽기 순서, `writeInterfaceToken` / `CHECK_INTERFACE` |
| Transaction code | `FIRST_CALL_TRANSACTION = 1`, `DUMP_TRANSACTION`, `INTERFACE_TRANSACTION` |
| 역방향 호출 | IBinder 를 Parcel 에 싣기 → Driver 의 handle 변환 → Client 도 Thread Pool 필요 |
| Thread Pool | `startThreadPool()` · `joinThreadPool()` · `BINDER_SET_MAX_THREADS` |

### Chapter 2. AIDL Native backend — 25분

| 주제 | 핵심 내용 |
|---|---|
| `aidl_interface` | srcs · `local_include_dir` · `backend { cpp / ndk / java / rust }` · `-cpp` / `-ndk` 라이브러리 |
| 생성 코드 | `BnX / BpX / IX` 헤더, `TRANSACTION_x`, `Status f(in…, out* _aidl_return)` |
| `binder::Status` | `ok()` · `fromExceptionCode(EX_SECURITY / EX_ILLEGAL_ARGUMENT)` · `fromServiceSpecificError` → Java 예외 매핑 |
| 안정성 | `unstable: true` (실습) vs `stability: "vintf"` + `versions` + `aidl_api/` 동결 (HAL) |
| NDK backend | `aidl::` 네임스페이스 · `ndk::ScopedAStatus` · `SharedRefBase::make` · `AServiceManager_*` · `fromBinder` |

### Chapter 3. System Service · 보안 · SELinux — 30분

| 주제 | 핵심 내용 |
|---|---|
| servicemanager 데몬 | `frameworks/native/cmds/servicemanager`, 이름↔handle, `mAccess->canAdd/canFind` (SELinux) |
| 호출자 확인 | `IPCThreadState::getCallingUid/Pid` — 커널 cred 기반, 위조 불가 · `AID_SYSTEM 1000`, App 10000+ |
| Java 등록 경로 | `SystemServer.run()` → `startBootstrap/Core/OtherServices` → `publishBinderService` → `ServiceManager.addService` → handle 0 |
| init.rc | `service <name> <path>` · `class / user / group / disabled / oneshot` · `start/stop` · `init.svc.<name>` |
| SELinux | 도메인 × 객체 × 권한 · `avc: denied` 읽기 · permissive / enforcing · `.te` · `service_contexts` · `file_contexts` · `audit2allow` |
| dumpsys | `IBinder.dump()` = `DUMP_TRANSACTION` |

### Chapter 4. HAL — AIDL HAL · VINTF — 30분

| 주제 | 핵심 내용 |
|---|---|
| HAL 세대 | Legacy `.so` → HIDL (hwbinder, Android 8~) → **AIDL HAL (binder, Android 11~, 16 기본)** |
| 경계 규칙 | system ↔ vendor 파티션 · `vendor: true` · vendor 는 `libbinder_ndk` 만 · `vendor_available` |
| 인스턴스 이름 | `<package>.<Interface>/<instance>` (예: `android.hardware.light.ILights/default`) |
| VINTF | vendor **manifest** ↔ system **compatibility matrix** · 부팅 시 대조 · `stability: vintf` 등록 거부 규칙 · `vintf_fragments` |
| Automotive 예고 | `android.hardware.automotive.vehicle.IVehicle/default` = VHAL (Day 4) |

---

## Part 2. 실습 (총 305분 + 선택 30분)

| # | 실습명 | 난이도 | 소요시간 | 챕터 |
|---|---|---|---|---|
| 2 | 최소 Native Binder Server — BBinder 하나 등록 | ★☆☆ | 20분 | Ch 1 |
| 3 | Client 측 Proxy 수작업 — ILedService · BpLedService · interface_cast | ★★☆ | 30분 | Ch 1 |
| 4 | Bn/Bp 대칭 완성 — BnLedService::onTransact · LedService | ★★☆ | 30분 | Ch 1 |
| 5 | Parcel 로 인자·반환값 전달 — ledOn(ratio) → int | ★★☆ | 20분 | Ch 1 |
| 6 | 역방향 호출 — Callback 을 writeStrongBinder 로 전달 | ★★★ | 30분 | Ch 1 |
| 7 | AIDL cpp backend 로 전환 — Bn/Bp 자동 생성 · binder::Status | ★★☆ | 40분 | Ch 2 |
| 8 | UID 기반 접근 제어 — getCallingUid · EX_SECURITY · su 1000 | ★★☆ | 30분 | Ch 2·3 |
| 9 | init.rc 로 Service 자동 시작 + SELinux 기초 | ★★★ | 40분 | Ch 3 |
| 10 | AIDL HAL 구조 따라 만들기 — NDK backend · vendor · VINTF | ★★★ | 45분 | Ch 4 |
| 11 | Java System Service 들여다보기 — SystemServer · ServiceManager · dumpsys | ★★☆ | 30분 | Ch 3 |
| 12 | App 에서 Native Service 호출 — transact 직접 · SecurityException (선택/과제) | ★★★ | 30분 | Ch 3 |

> 실습 2 → 7 은 같은 LED 서비스를 **raw BBinder → 수작업 Bn/Bp → AIDL 자동 생성** 순으로 발전시킨다. 실습 7 이 끝나면 "AIDL 이 대신 써 주는 코드" 를 전부 한 번씩 손으로 써 본 상태가 된다.

---

## Part 3. 데모 & 정리 (총 35분)

### 실습 13 (데모). Binder 호출 비용 측정 · strace 로 ioctl 관찰 — 15분 (강사 시연)
- 1000회 왕복 µs 측정 (30~80 µs/call) · `strace -e ioctl,mmap` 로 `BINDER_WRITE_READ`, 1 MB mmap, `BINDER_SET_MAX_THREADS` 확인 · `startThreadPool` 유무에 따른 tid 변화

### Day 2 Worksheet — 20분
- 객관식 · 단답형 · 빈칸 · 순번 · 선잇기 20문제, 별도 정답·해설

**출제 키워드**
```text
ProcessState / IPCThreadState
BBinder / BpBinder / BnInterface / BpInterface
DECLARE_META_INTERFACE / IMPLEMENT_META_INTERFACE / interface_cast
Parcel (writeInt32 · writeStrongBinder · writeInterfaceToken · CHECK_INTERFACE)
FIRST_CALL_TRANSACTION
startThreadPool / joinThreadPool
aidl_interface / cpp backend / ndk backend
binder::Status / EX_SECURITY / ScopedAStatus
getCallingUid / AID_SYSTEM
servicemanager / addService / checkService
init.rc / init_rc / start / stop / init.svc
SELinux avc denied / scontext / tcontext / permissive / enforcing / .te / service_contexts
AIDL HAL / vendor / libbinder_ndk / VINTF manifest / compatibility matrix / stability vintf
SystemServer / publishBinderService / dumpsys
```

---

## 준비물 체크리스트

**강사 / 사전 준비**
- [ ] Day 1 커스텀 Emulator 이미지 (userdebug, permissive) 배포 상태 확인
- [ ] 빌드 서버 swap 32GB, 조 단위 빌드 순서 공지
- [ ] `~/aosp/day2/Android.bp` 완성본 + 빈 소스 트리를 미리 만들어 둔 계정 (실습 1 시간 단축용)
- [ ] 실습 13 `my_client_bench` 빌드본, `strace` 가 Emulator 에 있는지 확인 (`adb shell which strace`)
- [ ] 실습 9 SELinux `.te` 시연용 정책 리빌드 이미지 (선택)
- [ ] Day 2 Worksheet 인쇄본 + 정답지

**수강생**
- [ ] Day 1 실습 4 의 배포 절차(`mm` → WinSCP → `adb push` → `chmod 755`) 숙지
- [ ] PuTTY 세션 3개 동시 사용 (server / client / trace)
