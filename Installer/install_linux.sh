#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

echo "============================================================"
echo "  OBYS // MOOD v1.0.0 - Linux VST3 Build & Installer"
echo "============================================================"

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j"$(nproc)"

mkdir -p "$HOME/.vst3"
cp -r "build/OBYS_MOOD_artefacts/Release/VST3/OBYS // MOOD.vst3" "$HOME/.vst3/"

echo "[SUCCESS] Installed OBYS // MOOD.vst3 to $HOME/.vst3/"
