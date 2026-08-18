#!/bin/bash
# ============================================================================
# Configura il build iOS di Decometer sul Mac. Genera un progetto Xcode; poi
# si apre in Xcode o si compila da riga di comando.
#
# Rispetto a decodium-mobile qui non serve nulla di precompilato: niente core
# del decoder, niente FFTW, niente Boost. Questa app e' Qt e basta, quindi il
# solo prerequisito e' il kit Qt per iOS.
#
# Prerequisiti:
#   - Xcode + Command Line Tools
#   - Qt for iOS (kit "ios") installato
#
# Uso:
#   export APPLE_TEAM_ID=XXXXXXXXXX   # Team ID Apple Developer (opzionale)
#   ./configure_ios.sh
# ============================================================================
set -e

QT_IOS="${QT_IOS:-$HOME/Qt/6.6.3/ios}"
SRC_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$SRC_DIR/build/ios}"
TEAM="${APPLE_TEAM_ID:-}"

echo "Qt iOS   : $QT_IOS"
echo "sorgenti : $SRC_DIR"
echo "build    : $BUILD_DIR"
[ -n "$TEAM" ] && echo "Team ID  : $TEAM" || echo "Team ID  : (non impostato: si firma da Xcode)"

if [ ! -x "$QT_IOS/bin/qt-cmake" ]; then
    echo "ERRORE: kit Qt per iOS non trovato in $QT_IOS" >&2
    echo "Installalo con Qt Maintenance Tool, oppure indica il percorso:" >&2
    echo "  QT_IOS=/percorso/Qt/6.x.y/ios ./configure_ios.sh" >&2
    exit 1
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

ARGS=(-G Xcode -DCMAKE_BUILD_TYPE=Release)
[ -n "$TEAM" ] && ARGS+=(-DAPPLE_TEAM_ID="$TEAM")

"$QT_IOS/bin/qt-cmake" "${ARGS[@]}" "$SRC_DIR"

echo
echo "Fatto. Ora:"
echo "  open $BUILD_DIR/Decometer.xcodeproj"
echo "oppure:"
echo "  cmake --build $BUILD_DIR --config Release"
