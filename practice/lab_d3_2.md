# [D3-2] Lab 3-1 — RRO 1차: 문자열·색상 오버레이

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★★★☆☆ | Day 3 Ch 8 · 필수 | Android Studio + 배포 `RROTarget`/`RROOverlay` · 🎬 ⑦㉒ 선시청 |

> 🎯 **실습 목표** — 타겟 APK를 **1바이트도 수정하지 않고** 문자열·색을 바꾸는 동적 RRO의 전 과정(overlayable 공개 → 오버레이 제작 → enable → 검증 → 해제)을 완주한다.

## Step 1. 타겟 앱 — 리소스 공개 (overlayable.xml)

🖱 `RROTarget/app/src/main/res/values/overlayable.xml`:

```xml
<resources>
    <overlayable name="EduOverlayable">
        <policy type="public">
            <item type="string" name="hello_text"/>
            <item type="color"  name="main_color"/>
        </policy>
    </overlayable>
</resources>
```

빌드·설치 후 기본 화면 확인(원본 문구/색 기록).

## Step 2. 오버레이 앱 — 같은 이름, 다른 값

🖱 `RROOverlay/app/src/main/AndroidManifest.xml`:

```xml
<manifest package="com.example.rrooverlay" ...>
    <application android:hasCode="false"/>
    <overlay android:targetPackage="com.example.rrotarget"
             android:targetName="EduOverlayable"
             android:priority="1"
             android:isStatic="false"/>
</manifest>
```

🖱 `res/values/strings.xml` / `colors.xml` 에 **동일 이름**으로:

```xml
<string name="hello_text">Hello, RRO! 🚀</string>
<color name="main_color">#FF6D00</color>
```

빌드 → 설치.

## Step 3. 활성화 → 눈으로 확인

```bat
adb shell cmd overlay list | findstr rro
adb shell cmd overlay enable com.example.rrooverlay
adb shell am force-stop com.example.rrotarget
:: 타겟 앱 재실행
```

✅ **예상 결과:** list에서 `[x] com.example.rrooverlay`, 화면 문구가 **"Hello, RRO! 🚀"** + 주황색

## Step 4. 내부 확인 + 해제

```bat
adb shell cmd overlay dump com.example.rrooverlay | findstr /i "state idmap"
adb shell cmd overlay disable com.example.rrooverlay
adb uninstall com.example.rrooverlay
```

✅ **예상 결과:** dump에 idmap 경로(/data/resource-cache/...) — disable 후 원래 화면 복귀

# 🏁 Pass 판정 체크리스트

- [ ] enable 전/후 화면 차이 확인 (문자열+색 2종)
- [ ] cmd overlay list의 [x] 표시와 dump의 idmap 라인 확보
- [ ] disable→원복까지 왕복 완료

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| enable 했는데 안 바뀜 | 앱 미재시작 / overlayable 미공개 | force-stop 후 재실행, Step 1의 item 목록 확인(🎬 ㉒ S5) |
| overlay list에 없음 | overlay 태그/targetPackage 오타 | Manifest 재확인 후 재설치 |
| INSTALL_FAILED... | 서명/targetName 불일치 | targetName=EduOverlayable 일치 확인 |

# 🚗 현업 활용 포인트

💡 지금 만든 구조가 **OEM 브랜딩 패키지의 원형**입니다. "타겟 무수정 + 정책적 공개 범위(overlayable)"라는 두 원칙만 지키면, 지역·트림별 UI 변형을 APK 하나로 배포할 수 있습니다.

---
*실습 D3-2 (26/36) · 다음: **D3-3 Lab 3-2 RRO 2차(Drawable)***
