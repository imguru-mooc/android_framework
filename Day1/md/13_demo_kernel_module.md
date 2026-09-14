# 실습 13 (데모). Android 16 Kernel Module 빌드 — ddk_module

> **소요시간:** 15분 (강사 시연) · **난이도:** ★★☆
> **환경:** Ubuntu 24 빌드 서버 (`~/android-kernel`, GKI 6.12) + Emulator

## 목표

- Android 16 GKI 커널이 어떻게 빌드되고 Emulator 이미지에 들어가는지 흐름을 본다.
- Kleaf `ddk_module` 로 커널 모듈(`.ko`)을 빌드해 Emulator에 `insmod` 한다.

> Android 16 에서 `kernel_module()` 은 **deprecated** 되어 빌드가 실패한다. `ddk_module()` 을 사용한다.

---

## Step 1. 커널 소스와 빌드 (사전 완료)

```bash
mkdir -p ~/android-kernel && cd ~/android-kernel
repo init -u https://android.googlesource.com/kernel/manifest -b common-android16-6.12 --depth=1
repo sync -c -j$(nproc) --no-tags -q

sudo apt install rsync -y
tools/bazel run //common:kernel_x86_64_dist -- --destdir=out/gki_modules
tools/bazel run //common-modules/virtual-device:virtual_device_x86_64_dist -- --destdir=out/goldfish_modules
```

## Step 2. AOSP prebuilt 커널 교체 → Emulator 이미지 리빌드 (사전 완료)

```bash
cd ~/aosp/prebuilts/qemu-kernel/x86_64/6.12
mv kernel-6.12 kernel-6.12-orig
cp ~/android-kernel/out/gki_modules/bzImage kernel-6.12

mv gki_modules gki_modules-orig && mkdir gki_modules
cp ~/android-kernel/out/gki_modules/*.ko gki_modules/

mv goldfish_modules goldfish_modules-orig && mkdir goldfish_modules
cp ~/android-kernel/out/goldfish_modules/*.ko goldfish_modules/

cd ~/aosp
source build/envsetup.sh
lunch sdk_car_x86_64-aosp_current-userdebug
m -j$(nproc)
m emu_img_zip
```

→ `sdk-repo-linux-system-images.zip` 을 WinSCP 로 받아 `run_custom_car_emulator.bat` 로 실행 (실습 4 참고).

## Step 3. 모듈 소스 작성

```bash
cd ~/android-kernel
mkdir -p my_modules/hello
```

**`my_modules/hello/hello_android.c`**

```c
// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Android Framework Training");
MODULE_DESCRIPTION("Hello Android 16 Kernel Module");
MODULE_VERSION("1.0");

static char msg_buf[256] = "Hello from Android 16 Kernel Module!\n";

static ssize_t hello_proc_read(struct file *file, char __user *buf,
                               size_t count, loff_t *pos)
{
    return simple_read_from_buffer(buf, count, pos, msg_buf, strlen(msg_buf));
}

static const struct proc_ops hello_proc_ops = {
    .proc_read = hello_proc_read,
};

static struct proc_dir_entry *proc_entry;

static int __init hello_android_init(void)
{
    pr_info("hello_android: Module loaded! (Android 16 Kernel)\n");
    proc_entry = proc_create("hello_android", 0444, NULL, &hello_proc_ops);
    if (!proc_entry) {
        pr_err("hello_android: Failed to create /proc entry\n");
        return -ENOMEM;
    }
    pr_info("hello_android: /proc/hello_android created successfully\n");
    return 0;
}

static void __exit hello_android_exit(void)
{
    if (proc_entry)
        proc_remove(proc_entry);
    pr_info("hello_android: Module unloaded. Goodbye!\n");
}

module_init(hello_android_init);
module_exit(hello_android_exit);
```

**`my_modules/hello/BUILD.bazel`**

```python
load("//build/kernel/kleaf:kernel.bzl", "ddk_module")

ddk_module(
    name = "hello_android",
    srcs = ["hello_android.c"],
    out = "hello_android.ko",
    kernel_build = "//common-modules/virtual-device:virtual_device_x86_64",
    deps = [],
)
```

> ⚠ 핵심
> - `ddk_module` 사용 (`kernel_module` 아님), `outs` 가 아닌 `out` (단일 문자열)
> - **Kbuild / Makefile 을 만들지 않는다** — `ddk_module` 이 자동 생성하며, 있으면 충돌
> - `kernel_build` 는 반드시 `virtual_device_x86_64` (x86_64 헤더)

## Step 4. 빌드

```bash
cd ~/android-kernel
tools/bazel build //my_modules/hello:hello_android --check_visibility=false --verbose_failures

# 영구 설정
echo "build --check_visibility=false" >> user.bazelrc
```

```bash
sudo apt install kmod file -y
KO=$(find bazel-bin/ -name "hello_android.ko" | head -1)
modinfo "$KO"      # vermagic 이 Emulator 커널과 일치해야 insmod 성공
file "$KO"         # ELF 64-bit LSB relocatable, x86-64
```

## Step 5. Emulator 에 로드

WinSCP 로 `.ko` 를 Windows 로 받은 뒤:

```bat
adb root
adb push hello_android.ko /data
adb shell
```

```bash
insmod /data/hello_android.ko
lsmod | grep hello
dmesg | grep hello_android
cat /proc/hello_android
# Hello from Android 16 Kernel Module!

rmmod hello_android
dmesg | tail -3
```

---

## 트러블슈팅

| 증상 | 원인 / 해결 |
|---|---|
| `kernel_module is deprecated` | `ddk_module` 로 교체 |
| `Kbuild already exists` / 이상한 빌드 오류 | 모듈 디렉토리의 Kbuild, Makefile 삭제 |
| `insmod: Invalid module format` | `vermagic` 불일치 — 커널과 모듈을 같은 소스에서 빌드 |
| `target ... is not visible` | `--check_visibility=false` |
| `insmod: Operation not permitted` | `adb root` 후 재시도, SELinux permissive 확인 |
