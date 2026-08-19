@echo off
REM Firma l'App Bundle di Decometer con la chiave di rilascio (upload key) del
REM Play Store. E' il gemello di decodium-mobile\androidapp\firma_aab.bat e
REM usa la STESSA chiave: una chiave puo' firmare piu' applicazioni, e
REM tenerne una sola significa averne una sola da custodire.
REM
REM ATTENZIONE, e vale una volta per tutte: dopo la prima pubblicazione la
REM chiave di caricamento non si cambia a piacere. Se va persa, per tornare a
REM pubblicare aggiornamenti bisogna chiedere a Google di azzerarla, e nel
REM frattempo l'app resta ferma.
REM
REM La chiave sta FUORI dal repository, in %USERPROFILE%\.android: una chiave
REM di firma versionata e' una chiave pubblica, e chiunque potrebbe firmare
REM aggiornamenti dell'app.
REM
REM La password NON sta qui: passala nella variabile d'ambiente
REM DECODIUM_KEYSTORE_PASS, oppure lo script la chiede.
REM
REM Si firma su un file di lavoro e lo si mette al posto del bundle buono SOLO
REM a firma riuscita. Prima no: un tentativo andato male - password sbagliata,
REM o anche solo un invio a vuoto sul prompt - porterebbe via il bundle
REM firmato la volta precedente, che e' l'unica copia buona.
setlocal
set "JAVA_HOME=C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot"
set "KEYSTORE=%USERPROFILE%\.android\decodium-upload.jks"
set "ALIAS=decodium-upload"
set "IN=C:\decodium-decometer\build\android\android-build\build\outputs\bundle\release\android-build-release.aab"
set "USCITA=C:\decodium-decometer\dist\playstore"
set "OUT=%USCITA%\Decometer-1.1.0.aab"
set "LAVORO=%USCITA%\Decometer-1.1.0.firma-in-corso.aab"

if not exist "%KEYSTORE%" (
    echo Chiave non trovata: %KEYSTORE%
    exit /b 1
)
if not exist "%IN%" (
    echo Bundle non trovato: %IN%
    echo Esegui prima aab_android.bat
    exit /b 1
)
if not exist "%USCITA%" mkdir "%USCITA%"
if "%DECODIUM_KEYSTORE_PASS%"=="" set /p DECODIUM_KEYSTORE_PASS=Password della chiave:

copy /y "%IN%" "%LAVORO%" >nul
"%JAVA_HOME%\bin\jarsigner.exe" -sigalg SHA256withRSA -digestalg SHA-256 -keystore "%KEYSTORE%" -storepass "%DECODIUM_KEYSTORE_PASS%" "%LAVORO%" "%ALIAS%"
if errorlevel 1 (
    echo.
    echo FIRMA NON RIUSCITA: password errata, oppure l'alias %ALIAS% non e' in questa chiave.
    echo Il bundle firmato in precedenza, se c'era, e' rimasto al suo posto.
    del "%LAVORO%" 2>nul
    exit /b 1
)

move /y "%LAVORO%" "%OUT%" >nul
"%JAVA_HOME%\bin\jarsigner.exe" -verify "%OUT%" >nul
if errorlevel 1 (
    echo VERIFICA FALLITA sul bundle appena firmato: %OUT%
    exit /b 1
)
echo.
echo Bundle firmato: %OUT%
for %%F in ("%OUT%") do echo Dimensione: %%~zF byte    Data: %%~tF
echo.
echo Da caricare nel Play Console. Il pacchetto e' com.ft2.decometer,
echo versione 1.1.0 (codice 2): a ogni pacchetto caricato il codice va
echo alzato in CMakeLists.txt, altrimenti il negozio rifiuta il caricamento.
endlocal
