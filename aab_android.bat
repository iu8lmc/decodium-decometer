@echo off
REM AAB per il Play Store (Android App Bundle, multi-ABI).
REM Il negozio non accetta piu' APK per le app nuove: vuole l'AAB, e da questo
REM ricava lui gli APK per ogni dispositivo. Va poi firmato con la chiave di
REM pubblicazione, che NON sta in questo repository.
set "JAVA_HOME=C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot"
set "ANDROID_SDK_ROOT=%LOCALAPPDATA%\Android\Sdk"
set "ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk"
set "ANDROID_NDK_ROOT=%LOCALAPPDATA%\Android\Sdk\ndk-r26d"
set "PATH=C:\msys64\mingw64\bin;%PATH%"
cd /d C:\decodium-decometer\build\android
cmake --build . --target decometer -j 12
cmake --build . --target aab -j 12
