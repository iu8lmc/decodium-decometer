cmake_minimum_required(VERSION 3.21)
project(Decometer VERSION 1.1 LANGUAGES CXX)

# LA VERSIONE SI SCRIVE QUI E BASTA. Prima compariva a mano in quattro posti —
# le proprieta' Android, il bundle iOS, la riga dei contatti nel QML — e infatti
# si e' disallineata: sul telefono si leggeva "Decometer 1.0" mentre il
# pacchetto era gia' 1.1.0.
set(DECOMETER_VERSION "1.2.0")

# Decometer standalone (Android + iOS): SOLO il frontalino RF di Decodium 4 e
# il client di rete che lo alimenta. Nessun decoder, nessun FFTW/Boost/Opus,
# nessuna seriale: legge gli strumenti della radio dal gateway DecoPort di
# Decodium 4, in rete locale — che si annuncia da se', quindi non c'e' un
# indirizzo da sapere a memoria, ed e' autenticato. E' per questo che il progetto sta in una cartella a
# se', separata da decodium-mobile: quell'app porta con se' tutto il motore di
# decodifica, che qui non serve a nulla e appesantirebbe solo il pacchetto.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(decometer
    main.cpp
    MeterBridge.cpp
    MeterBridge.hpp
    DecodeFeed.cpp
    DecodeFeed.hpp
    SpotFeed.cpp
    SpotFeed.hpp
    AntennaProbe.cpp
    AntennaProbe.hpp
    # Il client DecoPort, copiato da C:\decoport e non incluso da li'.
    # Un add_subdirectory con percorso assoluto compilerebbe solo su questa
    # macchina, e un'app che si costruisce anche per Android e iOS deve poter
    # partire da un albero solo. La copia va riallineata quando il protocollo
    # cambia: e' il prezzo, ed e' scritto qui perche' non si dimentichi.
    decoport/DecoPortPacket.cpp
    decoport/DecoPortPacket.h
    decoport/DecoPortLink.cpp
    decoport/DecoPortLink.h
    decoport/RadioLink.h
)

target_include_directories(decometer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/decoport)

qt_add_qml_module(decometer
    URI Decometer
    VERSION 1.0
    QML_FILES Main.qml Decometer.qml DecodeScreen.qml ClusterScreen.qml AntennaScreen.qml
)

target_link_libraries(decometer PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Network
)

# La versione arriva fin dentro il QML, che la mostra nei contatti.
target_compile_definitions(decometer PRIVATE
    DECOMETER_VERSION="${DECOMETER_VERSION}")

# ---- Android -----------------------------------------------------------------
if(ANDROID)
    # Il nome del pacchetto e' DEFINITIVO una volta pubblicato: non e' lo
    # stesso di Decodium 4 mobile (com.ft2.decodium) perche' e' un prodotto
    # separato sul negozio, con la propria scheda e le proprie recensioni.
    set_target_properties(decometer PROPERTIES
        QT_ANDROID_PACKAGE_NAME "com.ft2.decometer"
        QT_ANDROID_APP_NAME "Decometer"
        # Il codice di versione e' un numero che sale e basta: il negozio non
        # accetta due volte lo stesso, e non si torna indietro. Va alzato a
        # OGNI pacchetto caricato, anche quando il nome resta uguale.
        QT_ANDROID_VERSION_CODE 3
        QT_ANDROID_VERSION_NAME "${DECOMETER_VERSION}"
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
    # AudioToolbox e' per la vibrazione dell'allarme di ROS alto: su iOS
    # la vibrazione passa da AudioServicesPlaySystemSound.
    target_link_libraries(decometer PRIVATE "-framework UIKit" "-framework AudioToolbox")

    target_sources(decometer PRIVATE ios/Assets.xcassets)
    set_source_files_properties(ios/Assets.xcassets PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(decometer PROPERTIES
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon)
    set_target_properties(decometer PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "it.ft2.decometer"
        MACOSX_BUNDLE_BUNDLE_NAME "Decometer"
        MACOSX_BUNDLE_BUNDLE_VERSION "${DECOMETER_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${DECOMETER_VERSION}"
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
