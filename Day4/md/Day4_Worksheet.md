# Day 4 Worksheet

> **형식:** 객관식 8 · 단답형 5 · 빈칸 채우기 4 · 순번 지정 2 · 선잇기 1 (총 20문제) · **소요시간:** 25분
> **범위:** APEX · RRO/SRO · Car SystemUI RRO · SystemUI QS 타일 · Perfetto · WindowManager · CarService · CarPropertyManager · VHAL · 델타 배포

이름: ______________________

---

## 객관식 (Q1 ~ Q8)

**Q1.** APEX 모듈의 `file_contexts` 파일이 있어야 하는 위치는?

① APEX 모듈 디렉토리 안
② system/sepolicy/apex/<name>-file_contexts
③ /system/etc/selinux/
④ apex_manifest.json 과 같은 폴더

**Q2.** RRO 가 타겟 앱의 리소스를 덮어쓰려면 타겟 앱이 반드시 해야 하는 것은? (Android 11+)

① android:hasCode="false" 선언
② <overlayable> 로 허용 리소스 선언
③ priority 를 0 으로 설정
④ APK 를 /system/app 에 설치

**Q3.** `cmd overlay list` 에서 `---` 표시의 의미는?

① 활성화됨
② 비활성 · 활성화 가능
③ 매칭 실패 (idmap 없음) · 활성화 불가
④ 설치 중

**Q4.** Car SystemUI 오버레이를 `adb install` 로 설치하면 `---` 가 되는 이유는?

① minSdk 가 높아서
② Car SystemUI 가 <overlayable> 을 선언하지 않아 사용자(public) 정책 오버레이가 매칭되지 않기 때문
③ APK 서명이 달라서
④ SystemUI 가 실행 중이라서

**Q5.** `/system` 에 파일을 쓰기 위한 올바른 절차는?

① adb root 한 번
② adb remount 한 번
③ adb root → adb remount → adb reboot → adb root → adb remount (remount succeeded)
④ adb push 만 하면 된다

**Q6.** SystemUI 에 QS 타일 클래스를 만들었는데 패널에 안 보이고 SystemUI 가 크래시한다. 가장 먼저 의심할 것은?

① 아이콘 drawable 크기
② Dagger @Binds @IntoMap @StringKey 등록 누락
③ APK 서명
④ minSdk

**Q7.** 다른 앱 위에 떠 있는 창(`TYPE_APPLICATION_OVERLAY`)에 필요한 권한과 검사 위치는?

① INTERNET · 앱 내부
② SYSTEM_ALERT_WINDOW · WindowManagerService(system_server)
③ WRITE_SETTINGS · SurfaceFlinger
④ 권한 불필요

**Q8.** Android Automotive 에서 App 이 차량 속성을 읽을 때 실제 경로는?

① App → VHAL 직접
② App → CarPropertyManager → CarService(CarPropertyService) → VHAL(IVehicle)
③ App → Kernel 드라이버
④ App → SystemUI → VHAL

---

## 단답형 (Q9 ~ Q13)

**Q9.** APEX 가 부팅 시 마운트되는 경로의 형식은? (`/apex/` 뒤에 이름과 버전이 붙는 형태를 쓰시오, 예: 이름 com.example.simple 버전 1)

답: ______________________

**Q10.** RRO APK 의 AndroidManifest 에서 "코드가 없는 패키지" 임을 나타내는 `&lt;application&gt;` 속성은?

답: ______________________

**Q11.** Car SystemUI APK 에서 실제 리소스 이름(예: system_bar_clock_text_color)을 추출할 때 쓰는 명령은? (도구 이름 + 서브커맨드)

답: ______________________

**Q12.** Automotive 에서 차량 없이 CarService 에 이벤트를 주입하거나 주간/야간 모드를 바꾸는 셸 명령의 앞부분은? (예: `____ day-night-mode night`)

답: ______________________

**Q13.** Emulator 의 VHAL 값을 직접 바꾸는 `dumpsys` 대상 서비스 이름은? (`android.hardware.automotive.vehicle.____/default`)

답: ______________________

---

## 빈칸 채우기 (Q14 ~ Q17)

**Q14.** APEX 는 ( 1 ) 키(.pem/.avbpubkey)로 이미지를 검증하고 ( 2 ) 키(.pk8/.x509.pem)로 패키지를 서명한다 — 두 벌이 모두 필요하다.

(1) __________  (2) __________

**Q15.** RRO 의 `&lt;overlay&gt;` 에서 ( 1 ) 은 덮을 앱 패키지, ( 2 ) 는 그 앱의 overlayable 이름과 일치해야 한다. 같은 리소스를 두 RRO 가 덮으면 __priority__ 가 높은 것이 이긴다.

(1) __________  (2) __________

**Q16.** Java App 의 한 프레임은 UI Thread 의 ( 1 )(measure/layout/draw) 와 ( 2 ) 의 DrawFrames 두 Thread 로 처리되어 BufferQueue 를 거쳐 SurfaceFlinger 로 간다.

(1) __________  (2) __________

**Q17.** `CarPropertyManager.setProperty(Float.class, VehiclePropertyIds.HVAC_TEMPERATURE_SET, ( 1 ), 22.5f)` 의 세 번째 인자는 좌석 등 구역을 뜻하며 ( 2 )() 로 지원 값을 먼저 읽어야 한다.

(1) __________  (2) __________

---

## 순번 지정 (Q18 ~ Q19)

**Q18.** Car SystemUI 색을 RRO 로 바꾸는 순서를 매기시오.

- ( &nbsp; ) cmd overlay enable → killall com.android.systemui
- ( &nbsp; ) aapt dump resources 로 실제 리소스 이름 추출
- ( &nbsp; ) 2단계 remount 후 /system/app/<Overlay>/ 에 push → reboot
- ( &nbsp; ) targetName 없는 <overlay> + colors.xml 로 APK 생성
- ( &nbsp; ) cmd overlay list 에서 --- 와 mTargetOverlayableName: null 확인

**Q19.** HvacSimulator 의 온도 버튼 한 번이 화면에 반영되기까지 순서를 매기시오.

- ( &nbsp; ) CarPropertyService 가 권한 검사 후 IVehicle.setValues 호출
- ( &nbsp; ) VHAL 이 값을 바꾸고 onPropertyEvent 로 CarService 에 통지
- ( &nbsp; ) App 이 ICarProperty.setProperty Binder 호출
- ( &nbsp; ) onChangeEvent(Binder Thread) → runOnUiThread → Choreographer#doFrame → SF
- ( &nbsp; ) CarService 가 subscriber(App) 에 onEvent 전달

---

## 선잇기 (Q20)

**Q20.** 바꾸려는 것과 필요한 배포 절차를 알맞게 연결하시오.

| 구성 요소 | | 역할 |
|---|---|---|
| Native 바이너리 / .so | · &nbsp;&nbsp;&nbsp; · | m emu_img_zip + Emulator 재시작 |
| SystemUI 색·문자열 | · &nbsp;&nbsp;&nbsp; · | m + adb push /data |
| SystemUI 코드 (QS 타일) | · &nbsp;&nbsp;&nbsp; · | APEX (.apex push + reboot) |
| 독립 배포 모듈 | · &nbsp;&nbsp;&nbsp; · | RRO (APK + cmd overlay enable) |
| 커널 · VHAL 속성 추가 | · &nbsp;&nbsp;&nbsp; · | m SystemUI + APK push + killall |

---
---

# Day 4 Worksheet — 정답 및 해설

**1. ②**

Soong 은 file_contexts 를 system/sepolicy/apex/{APEX 이름}-file_contexts 에서만 찾는다. 모듈 안에 두면 "should be under system/sepolicy" 오류.

**2. ②**

타겟이 overlayable.xml 의 <policy type="public"> 에 넣은 리소스만 사용자 설치 RRO 가 덮을 수 있다. 목록 밖 리소스는 무시된다.

**3. ③**

[x] 활성, [ ] 매칭 성공·비활성, --- 매칭 실패(STATE_NO_IDMAP). targetName 불일치나 overlayable 미선언이 원인이다.

**4. ②**

사용자 설치 오버레이는 policy public 이라 overlayable 이 필수다. /system/app 에 설치하면 policy system 이 되어 overlayable 없이도 매칭된다.

**5. ③**

1차 remount 는 verity 해제·overlayfs 설정(재부팅 필요), 재부팅 후 2차 remount 가 실제 쓰기 마운트다. 2차를 빼면 Read-only file system.

**6. ②**

QSFactory 는 Dagger 맵으로 spec → 타일을 만든다. 등록이 빠지면 config.xml 의 spec 을 생성하지 못해 예외가 난다. logcat -s AndroidRuntime 에서 spec 이름을 찾는다.

**7. ②**

WMS 의 addWindow 가 호출자 uid 의 SYSTEM_ALERT_WINDOW 를 검사한다. 없으면 BadTokenException. Emulator 는 appops set … allow 로 준다.

**8. ②**

App 은 CarService(uid system) 만 본다. CarPropertyService 가 권한을 검사하고 PropertyHalService 가 AIDL HAL IVehicle 을 부른다. Day 2 의 HAL 규칙 그대로다.

**9. /apex/com.example.simple@1**

apexd 가 /apex/<name>@<version> 에 loop mount 하고 /apex/<name> 심볼릭 링크를 만든다.

**10. android:hasCode="false"**

RRO 는 리소스만 담는다. hasCode="false" 로 Activity·코드 없이 설치된다.

**11. aapt dump resources**

aapt dump resources CarSystemUI.apk | findstr color 로 Car 전용 이름을 뽑는다. Phone SystemUI 와 이름이 달라 추측하면 안 바뀐다.

**12. cmd car_service**

cmd car_service inject-vhal-event / day-night-mode / get-property-value / inject-key — CarService 의 onShellCommand 다.

**13. IVehicle**

dumpsys android.hardware.automotive.vehicle.IVehicle/default --set <propId> -a <areaId> -f <value>. HAL 인스턴스 이름 규칙 <pkg>.<Interface>/<instance> 그대로다.

**14. (1) AVB / (2) APK**

apex_key 가 AVB(부팅 검증), android_app_certificate 가 APK 서명이다.

**15. (1) targetPackage / (2) targetName**

Car SystemUI 처럼 overlayable 이 없으면 targetName 을 제거하고 /system/app 에 설치한다.

**16. (1) Choreographer#doFrame / (2) RenderThread**

Perfetto 에서 두 트랙으로 Jank 원인을 가른다. Day 3 의 Native 예제는 Thread 하나였다.

**17. (1) areaId / (2) getCarPropertyConfig**

Emulator 의 HVAC 온도 areaId 는 좌석 그룹 49/68 이다. 잘못된 areaId 나 범위 밖 값은 IllegalArgumentException.

**18. cmd overlay list 에서 --- 와 mTargetOverlayableName: null 확인 → aapt dump resources 로 실제 리소스 이름 추출 → targetName 없는 <overlay> + colors.xml 로 APK 생성 → 2단계 remount 후 /system/app/<Overlay>/ 에 push → reboot → cmd overlay enable → killall com.android.systemui**

--- 확인 → aapt 로 이름 → targetName 없는 APK → /system/app 설치·reboot → enable·killall.

**19. App 이 ICarProperty.setProperty Binder 호출 → CarPropertyService 가 권한 검사 후 IVehicle.setValues 호출 → VHAL 이 값을 바꾸고 onPropertyEvent 로 CarService 에 통지 → CarService 가 subscriber(App) 에 onEvent 전달 → onChangeEvent(Binder Thread) → runOnUiThread → Choreographer#doFrame → SF**

App → CarService(권한·HAL 호출) → VHAL → 역방향 통지 → App 콜백 → UI Thread → SF. 실습 13 의 Perfetto flow 와 같다.

**20. Native 바이너리 / .so ↔ m + adb push /data, SystemUI 색·문자열 ↔ RRO (APK + cmd overlay enable), SystemUI 코드 (QS 타일) ↔ m SystemUI + APK push + killall, 독립 배포 모듈 ↔ APEX (.apex push + reboot), 커널 · VHAL 속성 추가 ↔ m emu_img_zip + Emulator 재시작**

델타 배포 판단표. 이미지를 다시 굽는 것은 커널·기본 탑재 RRO·VHAL 속성 추가뿐이다.
