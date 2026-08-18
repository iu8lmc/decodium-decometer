@echo off
REM APK di prova (multi-ABI). Da firmare con la debug keystore e installare
REM con adb; per il negozio serve invece l'AAB (aab_android.bat).
set "JAVA_HOME=C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot"
set "ANDROID_SDK_ROOT=%LOCALAPPDATA%\Android\Sdk"
set "ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk"
set "ANDROID_NDK_ROOT=%LOCALAPPDATA%\Android\Sdk\ndk-r26d"
set "PATH=C:\msys64\mingw64\bin;%PATH%"
cd /d C:\decodium-decometer\build\android
cmake --build . --target decometer -j 12
cmake --build . --target apk -j 12
