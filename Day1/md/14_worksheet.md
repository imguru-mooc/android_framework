# Day 1 Worksheet

> **형식:** 객관식 8 · 단답형 5 · 빈칸 채우기 4 · 순번 지정 2 · 선잇기 1 (총 20문제) · **소요시간:** 25분
> **범위:** 환경 설정 · Android Architecture · Boot · Binder · AIDL · Messenger · Kernel Module

이름: ______________________

---

## 객관식 (Q1 ~ Q8)

**Q1.** Android 에서 PID 1 인 Process 는?

① Zygote
② system_server
③ init
④ ServiceManager

**Q2.** Binder 의 주요 목적은?

① 파일 압축
② Process 간 통신 (IPC)
③ 화면 Rendering
④ 메모리 할당

**Q3.** AIDL Method 를 `oneway` 로 선언했을 때의 동작으로 옳은 것은?

① reply 를 받을 때까지 블로킹된다
② Driver 에 넣자마자 즉시 반환된다
③ 반환값을 가질 수 있다
④ 같은 Process 에서만 호출 가능하다

**Q4.** `TransactionTooLargeException` 이 발생하는 이유는?

① AIDL 이 byte[] 를 지원하지 않아서
② Process 당 Transaction Buffer 가 약 1 MB 로 제한되어서
③ Binder Thread 가 부족해서
④ SELinux 가 차단해서

**Q5.** Messenger IPC 와 AIDL 의 차이로 옳은 것은?

① Messenger 는 Binder 를 사용하지 않는다
② Messenger 는 Handler 큐로 단일 Thread 에서 순차 처리한다
③ AIDL 은 별도 Process 에서 사용할 수 없다
④ Messenger 는 .aidl 파일이 필요하다

**Q6.** `adb root` 가 "cannot run as root in production builds" 로 실패하는 가장 흔한 원인은?

① adb 버전이 오래됨
② Google Play 시스템 이미지를 사용함
③ USB 디버깅이 꺼져 있음
④ Emulator RAM 부족

**Q7.** `Stub.asInterface(binder)` 가 Proxy 가 아닌 Stub 객체 자체를 반환하는 경우는?

① Service 가 oneway 로 선언된 경우
② Client 와 Service 가 같은 Process 인 경우
③ BIND_AUTO_CREATE 를 쓴 경우
④ Service 가 system_server 에 있는 경우

**Q8.** Android 16 GKI 커널 모듈 빌드 시 `BUILD.bazel` 에 사용하는 규칙은?

① kernel_module()
② cc_binary()
③ ddk_module()
④ apex()

---

## 단답형 (Q9 ~ Q13)

**Q9.** `init.zygote64_32.rc` 에서 Zygote 가 system_server 를 fork 하도록 지정하는 실행 옵션은?

답: ______________________

**Q10.** AOSP 소스 루트에서 `m`, `mm`, `lunch` 같은 빌드 함수를 쉘에 로드하는 명령은?

답: ______________________

**Q11.** 현재 디렉토리의 모듈(Android.bp)만 빌드하는 AOSP 명령은?

답: ______________________

**Q12.** Service Process 가 죽었을 때 `DeathRecipient.binderDied()` 가 호출되는 Client 측 Thread 의 종류는?

답: ______________________

**Q13.** Framework 클래스를 preload 한 뒤 `fork()` 로 모든 App Process 와 system_server 를 만드는 Process 의 이름은?

답: ______________________

---

## 빈칸 채우기 (Q14 ~ Q17)

**Q14.** AIDL 을 빌드하면 Server 가 상속해 `onTransact()` 를 받는 클래스는 ( 1 ), Client 가 받아 `transact()` 를 호출하는 대리 객체는 ( 2 ) 이다.

(1) __________  (2) __________

**Q15.** Process 당 Binder Transaction Buffer 크기는 약 ( 1 ) MB 이며, 대용량 데이터는 ( 2 ) 로 FD 만 전달한다.

(1) __________  (2) __________

**Q16.** Service 를 별도 Process 에서 실행해 실제 IPC 가 발생하게 하려면 Manifest 의 `&lt;service&gt;` 에 ( 1 ) 속성을 지정하고, Service Process 사망 통지를 등록하는 IBinder Method 는 ( 2 ) 이다.

(1) __________  (2) __________

**Q17.** Android 16 에서 `lunch` 타겟은 ( 1 )-release-variant 의 3-파트 형식이며, 실습 타겟은 `sdk_car_x86_64-aosp_current-`( 2 ) 이다.

(1) __________  (2) __________

---

## 순번 지정 (Q18 ~ Q19)

**Q18.** Android Boot Sequence 를 순서대로 번호를 매기시오.

- ( &nbsp; ) Zygote 시작 (Framework preload)
- ( &nbsp; ) init 실행 (PID 1, init.rc 해석)
- ( &nbsp; ) Bootloader 가 Kernel 을 로드
- ( &nbsp; ) system_server fork
- ( &nbsp; ) Linux Kernel 부팅 (Binder Driver 로드)

**Q19.** 빌드 서버에서 만든 Native 바이너리를 Emulator 에서 실행하는 절차를 순서대로 번호를 매기시오.

- ( &nbsp; ) adb push my_sp /data
- ( &nbsp; ) mm -j$(nproc)
- ( &nbsp; ) $OUT/system/bin/my_sp 를 Windows 로 다운로드
- ( &nbsp; ) adb shell chmod 755 /data/my_sp
- ( &nbsp; ) lunch sdk_car_x86_64-aosp_current-userdebug
- ( &nbsp; ) adb shell /data/my_sp

---

## 선잇기 (Q20)

**Q20.** Android 구성 요소와 역할을 알맞게 연결하시오.

| 구성 요소 | | 역할 |
|---|---|---|
| init | · &nbsp;&nbsp;&nbsp; · | Binder Service 이름 등록·조회 (service list) |
| Zygote | · &nbsp;&nbsp;&nbsp; · | AMS·WMS·PMS 등 System Service 실행 |
| system_server | · &nbsp;&nbsp;&nbsp; · | PID 1, init.rc 해석 후 Service 실행 |
| ServiceManager | · &nbsp;&nbsp;&nbsp; · | /dev/binder, Process 간 1-copy 전달 |
| Binder Driver | · &nbsp;&nbsp;&nbsp; · | Framework preload 후 fork 로 App 생성 |

---
---

# Day 1 Worksheet — 정답 및 해설

**1. ③**

init 은 커널이 실행하는 첫 User Process 로 init.rc 를 해석해 Zygote 등 Service 를 띄운다.

**2. ②**

Binder 는 Android 표준 IPC 이다. App 과 system_server 의 통신은 모두 Binder Driver 를 거친다.
① 파일 압축 — Binder 와 무관 ③ Rendering — SurfaceFlinger ④ 메모리 할당 — 커널

**3. ②**

oneway 는 reply 가 없어 즉시 반환된다. 반환값은 가질 수 없고 결과는 Callback 인터페이스로 받는다.

**4. ②**

Buffer 는 Process 당 1 MB 이고 모든 진행 중 Transaction 이 공유한다. 대용량은 ParcelFileDescriptor 로 FD 만 전달한다.

**5. ②**

Messenger 도 내부는 Binder 지만 Handler 로 감싸 단일 Thread 순차 처리한다. AIDL 은 여러 Binder Thread 에서 병렬 처리된다.

**6. ②**

Google Play 이미지는 user 빌드라 adbd 를 root 로 재시작할 수 없다. Google APIs 또는 직접 빌드한 userdebug 이미지를 쓴다.

**7. ②**

queryLocalInterface() 로 같은 Process 임을 확인하면 Stub 을 그대로 반환해 Binder 를 거치지 않는다. android:process 를 제거하면 "Same process: YES" 가 나오는 이유다.

**8. ③**

Android 16 Kleaf 에서 kernel_module() 은 deprecated 이며 ddk_module() 을 사용한다. Kbuild/Makefile 은 자동 생성된다.

**9. --start-system-server**

service zygote /system/bin/app_process64 … --zygote --start-system-server. 이 옵션 때문에 Zygote 가 preload 직후 system_server 를 fork 한다.

**10. source build/envsetup.sh**

터미널 세션마다 source build/envsetup.sh 를 실행한 뒤 lunch 로 타겟을 선택해야 한다.

**11. mm**

m 은 전체 빌드, mm 은 현재 디렉토리 모듈만 빌드한다. 실습에서 my_sp, led_auth_service 등을 이 명령으로 빌드한다.

**12. Binder Thread**

Driver 가 BR_DEAD_BINDER 를 보내면 Client 의 Binder Thread 에서 호출된다. UI 갱신은 runOnUiThread() 가 필요하다.

**13. Zygote**

init.rc 의 service zygote 정의로 시작되며, --start-system-server 옵션으로 system_server 도 fork 한다.

**14. (1) Stub / (2) Proxy**

Stub 은 Binder 를 상속한 Server 측, Stub.Proxy 는 Client 측이다. 둘 다 같은 인터페이스를 구현한다.

**15. (1) 1 / (2) ParcelFileDescriptor (Pipe)**

1 MB 를 모든 진행 중 Transaction 이 공유하므로 실질 한계는 그 절반 정도다. ParcelFileDescriptor.createPipe() 를 쓰면 Parcel 에는 FD 만 실린다.

**16. (1) android:process / (2) linkToDeath()**

android:process=":calc_remote" 로 Service 를 분리하면 Proxy 경로로 Binder IPC 가 발생한다. linkToDeath() 로 등록한 DeathRecipient 는 Process 종료 시 Binder Thread 에서 호출된다.

**17. (1) product / (2) userdebug**

Android 14 의 2-파트(sdk_car_x86_64-userdebug) 와 달리 release 가 추가되었다. userdebug 여야 adb root 가 가능하다.

**18. Bootloader 가 Kernel 을 로드 → Linux Kernel 부팅 (Binder Driver 로드) → init 실행 (PID 1, init.rc 해석) → Zygote 시작 (Framework preload) → system_server fork**

Bootloader → Linux Kernel → init → Zygote → system_server. Zygote 가 --start-system-server 옵션으로 system_server 를 fork 한다.

**19. lunch sdk_car_x86_64-aosp_current-userdebug → mm -j$(nproc) → $OUT/system/bin/my_sp 를 Windows 로 다운로드 → adb push my_sp /data → adb shell chmod 755 /data/my_sp → adb shell /data/my_sp**

lunch → mm → 결과물 다운로드 → adb push → chmod 755 → 실행. Day 2~4 Native 실습의 공통 배포 절차다.

**20. init ↔ PID 1, init.rc 해석 후 Service 실행, Zygote ↔ Framework preload 후 fork 로 App 생성, system_server ↔ AMS·WMS·PMS 등 System Service 실행, ServiceManager ↔ Binder Service 이름 등록·조회 (service list), Binder Driver ↔ /dev/binder, Process 간 1-copy 전달**

init(PID 1) → Zygote(preload·fork) → system_server(System Service 실행). ServiceManager 는 이름↔Binder 테이블, Binder Driver 는 커널에서 실제 데이터를 전달한다.
