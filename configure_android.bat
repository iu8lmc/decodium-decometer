@echo off
REM Configura la build Android multi-ABI di Decometer.
REM
REM Rispetto a decodium-mobile qui non serve passare CORE_LIB ne' FFTW3F_PREFIX:
REM questa app non ha decoder, quindi non ha nulla da linkare oltre a Qt. E'
REM per lo stesso motivo che si configurano tutte e tre le architetture in un
REM colpo solo senza doversi prima compilare le dipendenze per ciascuna.
REM
REM armeabi-v7a c'e' perche' telefoni molto diffusi e molto economici hanno un
REM processore a 64 bit ma un sistema Android a 32: per il Play Store contano
REM come armeabi-v7a, e senza quella libreria dentro il negozio risponde
REM "dispositivo non compatibile".
set "JAVA_HOME=C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot"
set "ANDROID_SDK_ROOT=%LOCALAPPDATA%\Android\Sdk"
set "ANDROID_HOME=%LOCALAPPDATA%\Android\Sdk"
set "ANDROID_NDK_ROOT=%LOCALAPPDATA%\Android\Sdk\ndk-r26d"
set "PATH=C:\msys64\mingw64\bin;%PATH%"
if not exist C:\decodium-decometer\build\android mkdir C:\decodium-decometer\build\android
cd /d C:\decodium-decometer\build\android
call "C:\Qt\6.6.3\android_arm64_v8a\bin\qt-cmake.bat" -G Ninja ^
 -DCMAKE_BUILD_TYPE=Release ^
 -DQT_HOST_PATH=C:/Qt/6.6.3/mingw_64 ^
 -DQT_ANDROID_ABIS="arm64-v8a;armeabi-v7a;x86_64" ^
 C:\decodium-decometer
