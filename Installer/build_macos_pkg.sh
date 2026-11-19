#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

APP_NAME="OBYS MOOD"
VERSION="1.0.0"
PKG_ID="agency.obys.dsp.mood"
BUILD_DIR="build"
STAGING_DIR="build/pkg_staging"
DIST_DIR="dist"

echo "============================================================"
echo "  $APP_NAME v$VERSION - macOS Universal (.pkg) Builder"
echo "============================================================"

echo "[1/4] Configuring Universal Binary (arm64 + x86_64)..."
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

echo "[2/4] Building VST3, AU (Component) & Standalone..."
cmake --build "$BUILD_DIR" --config Release -j"$(sysctl -n hw.ncpu)"

echo "[3/4] Staging payload for macOS Installer..."
rm -rf "$STAGING_DIR" "$DIST_DIR"
mkdir -p "$STAGING_DIR/Library/Audio/Plug-Ins/VST3"
mkdir -p "$STAGING_DIR/Library/Audio/Plug-Ins/Components"
mkdir -p "$STAGING_DIR/Applications"
mkdir -p "$DIST_DIR"

cp -R "$BUILD_DIR/OBYS_MOOD_artefacts/Release/VST3/$APP_NAME.vst3" "$STAGING_DIR/Library/Audio/Plug-Ins/VST3/"
cp -R "$BUILD_DIR/OBYS_MOOD_artefacts/Release/AU/$APP_NAME.component" "$STAGING_DIR/Library/Audio/Plug-Ins/Components/"
cp -R "$BUILD_DIR/OBYS_MOOD_artefacts/Release/Standalone/$APP_NAME.app" "$STAGING_DIR/Applications/"

echo "[4/4] Building macOS .pkg installer..."
pkgbuild \
  --root "$STAGING_DIR" \
  --identifier "$PKG_ID" \
  --version "$VERSION" \
  --install-location "/" \
  "$DIST_DIR/OBYS_MOOD_Installer_v${VERSION}_macOS.pkg"

echo ""
echo "[SUCCESS] macOS Installer generated:"
echo "  -> $DIST_DIR/OBYS_MOOD_Installer_v${VERSION}_macOS.pkg"
