@echo off
set EMULATOR=%LOCALAPPDATA%\Android\Sdk\emulator\emulator.exe
set AVD_NAME=Automotive14
set IMAGES=C:\aaos14\x86_64

%EMULATOR% ^
-avd %AVD_NAME% ^
-writable-system ^
-no-snapshot-load ^
-no-snapshot-save ^
-verbose ^
-show-kernel ^
-qemu -append "androidboot.selinux=permissive"