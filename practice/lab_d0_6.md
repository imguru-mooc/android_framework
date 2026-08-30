# [D0-6] JNI 실습용 JDK 11 확인 — 빌드 서버

| 난이도 | 수행 시점 | 준비물 |
|---|---|---|
| ★☆☆☆☆ | Day 0 ★전원 필수 | 서버 접속(D0-3/4) |

> 🎯 **실습 목표** — Day 2 JNI_test(First~Fourth)가 요구하는 **OpenJDK 11**과 `javac -h`, JVM 공유 라이브러리(`libjvm.so`) 위치를 미리 확인한다.

## Step 1. 버전 확인

```bash
java -version
javac -version
```

✅ **예상 결과:** 둘 다 `11.x` — 다른 버전이 뜨면 Step 2로

## Step 2. (필요 시) JDK 11로 전환

```bash
sudo update-alternatives --config java
sudo update-alternatives --config javac
# 목록에서 java-11-openjdk 선택
```

💡 sudo 권한이 없으면 조교에게 요청하거나, `export PATH=/usr/lib/jvm/java-11-openjdk-amd64/bin:$PATH` 를 `~/.bashrc`에 추가.

## Step 3. `javac -h` 동작 테스트 (First 실습 사전 점검)

```bash
mkdir -p ~/jni_check && cd ~/jni_check
cat > Hello.java << 'EOF'
public class Hello {
    public native void foo();
}
EOF
javac -h . Hello.java
ls
```

✅ **예상 결과:** `Hello.class` 와 **`Hello.h`** 생성 — 헤더 안에 `Java_Hello_foo` 선언 확인

## Step 4. libjvm.so 위치 확인 (Fourth·Invocation API 대비)

```bash
find /usr/lib/jvm -name "libjvm.so" 2>/dev/null
```

✅ **예상 결과:** `/usr/lib/jvm/java-11-.../lib/server/libjvm.so` 한 줄 — 이 경로가 Day 2 Fourth의 `-L`/`LD_LIBRARY_PATH`에 들어갑니다.

# 🏁 Pass 판정 체크리스트

- [ ] `java -version` = 11.x
- [ ] `javac -h .` 로 Hello.h 생성 성공
- [ ] `libjvm.so` 경로 기록

# 🔧 자주 발생하는 오류

| 증상 | 원인 | 해결 |
|---|---|---|
| `javah: command not found` | JDK 10+에서 javah 삭제됨 | **javah가 아니라 `javac -h`** 를 씁니다 (자료의 표준) |
| 버전이 17/21로 나옴 | 시스템 기본이 상위 JDK | Step 2 전환 또는 PATH 우선순위 조정 |

# 🚗 현업 활용 포인트

💡 AOSP 자체도 특정 JDK 버전을 고정해 빌드합니다. "도구 버전을 먼저 못 박는" 오늘의 점검이, 내일의 "내 PC에선 됐는데요"를 없앱니다.

---
*실습 D0-6 (6/36) · 다음: **D1-0 환경 삼중 점검 + tmux 3명령***
