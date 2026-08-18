cmake_minimum_required(VERSION 3.21)
project(Decometer VERSION 1.0 LANGUAGES CXX)

# Decometer standalone (Android + iOS): SOLO il frontalino RF di Decodium 4 e
# il client di rete che lo alimenta. Nessun decoder, nessun FFTW/Boost/Opus,
# nessuna seriale: legge potenza/ROS/ALC dal server CAT condiviso di Decodium
# 4 (o da un qualunque server compatibile col protocollo rigctl di Hamlib) via
# TCP, in rete locale. E' per questo che il progetto sta in una cartella a
# se', separata da decodium-mobile: quell'app porta con se' tutto il motore di
# decodifica, che qui non serve a nulla e appesantirebbe solo il pacchetto.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Network)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(decometer
    main.cpp
    MeterBridge.cpp
    MeterBridge.hpp
)

qt_add_qml_module(decometer
    URI Decometer
    VERSION 1.0
    QML_FILES Main.qml Decometer.qml
)

target_link_libraries(decometer PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Network
)

# ---- Android -----------------------------------------------------------------
if(ANDROID)
    # Il nome del pacchetto e' DEFINITIVO una volta pubblicato: non e' lo
    # stesso di Decodium 4 mobile (com.ft2.decodium) perche' e' un prodotto
    # separato sul negozio, con la propria scheda e le proprie recensioni.
    set_target_properties(decometer PROPERTIES
        QT_ANDROID_PACKAGE_NAME "com.ft2.decometer"
        QT_ANDROID_APP_NAME "Decometer"
        QT_ANDROID_VERSION_CODE 1
        QT_ANDROID_VERSION_NAME "1.0.0"
        QT_ANDROID_TARGET_SDK_VERSION 36
        QT_ANDROID_MIN_SDK_VERSION 24
        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
    )

    # Pagine di memoria da 16 KB, richieste dal Play Store dal 1 febbraio 2027
    # per ogni app che punta ad Android 15+. Vedi la stessa nota in
    # decodium-mobile/androidapp/CMakeLists.txt: qui la libreria dell'app e'
    # piccola, ma il requisito e' dell'intero pacchetto, non solo del motore
    # di decodifica che questa app non ha.
    target_link_options(decometer PRIVATE
        "-Wl,-z,max-page-size=16384"
        "-Wl,-z,common-page-size=16384"
    )
endif()

# ---- iOS ----------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(MACOSX_BUNDLE_EXECUTABLE_NAME decometer)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER it.ft2.decometer)

    # Schermo sempre acceso mentre si guarda il quadrante: l'unico pezzo
    # nativo che serve qui, tolto da IosPlatform.mm di decodium-mobile (che
    # ha anche portachiavi e sessione audio, inutili per un'app di sola
    # lettura CAT).
    target_sources(decometer PRIVATE ios/IosPlatform.mm)
    target_link_libraries(decometer PRIVATE "-framework UIKit")

    target_sources(decometer PRIVATE ios/Assets.xcassets)
    set_source_files_properties(ios/Assets.xcassets PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(decometer PROPERTIES
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon)
    set_target_properties(decometer PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "it.ft2.decometer"
        MACOSX_BUNDLE_BUNDLE_NAME "Decometer"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "it.ft2.decometer"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        # Imposta il proprio Team ID Apple Developer (10 caratteri):
        #   qt-cmake ... -DAPPLE_TEAM_ID=XXXXXXXXXX   oppure in Xcode > Signing.
    )
    if(APPLE_TEAM_ID)
        set_target_properties(decometer PROPERTIES
            XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${APPLE_TEAM_ID}")
    endif()
endif()
