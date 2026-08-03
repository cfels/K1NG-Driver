#!/usr/bin/env bash

set -e

# VARS
PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="$PROJECT_DIR/output"
BUILD_DIR="$PROJECT_DIR/build"
STAGE_DIR="$PROJECT_DIR/.cache/install-stage"
JOBS=$(nproc)

cd "$PROJECT_DIR"

# cleanup stuff
echo "cleanup!"
make clean
rm -rf "$BUILD_DIR"
rm -rf "$OUTPUT_DIR"
rm -rf "$STAGE_DIR"
mkdir -p "$OUTPUT_DIR/scripts"
mkdir -p "$OUTPUT_DIR/driver_bin"
echo ""

# build driver
echo "build driver"
nix-shell --run "make clean && make"
cp "$PROJECT_DIR/k1ng_driver" "$PROJECT_DIR/src/ui/driver_bin"
echo ""

# build ui
echo "building UI"
nix-shell --run "cmake -S . -B build && cmake --build build -j $JOBS && cmake --install build --prefix '$STAGE_DIR'"
ln -sf build/compile_commands.json compile_commands.json
echo ""

# stich together
echo "stich together"
cp "$STAGE_DIR/bin/k1ng-driver/k1ngui" "$OUTPUT_DIR"
cp "$STAGE_DIR/bin/k1ng-driver/libm3shapes.so" "$OUTPUT_DIR"
cp "$STAGE_DIR/bin/k1ng-driver/driver_bin/k1ng_driver" "$OUTPUT_DIR/driver_bin"
cp "$STAGE_DIR/share/applications/com.moxiu.k1ng.desktop" "$OUTPUT_DIR"
cp "$STAGE_DIR/bin/k1ng-driver/logo.png" "$OUTPUT_DIR"
cp "$PROJECT_DIR/scripts/install.sh" "$OUTPUT_DIR"
cp "$PROJECT_DIR/scripts/applyrules.sh" "$OUTPUT_DIR/scripts"
cp "$PROJECT_DIR/src/ui/scripts/run.sh" "$OUTPUT_DIR/scripts"

find "$PROJECT_DIR/scripts" "$PROJECT_DIR/src/ui/scripts" "$OUTPUT_DIR" -type f -name "*.sh" -exec chmod +x {} +
chmod +x "$OUTPUT_DIR/k1ngui"
chmod +x "$OUTPUT_DIR/driver_bin/k1ng_driver"

rm -rf "$STAGE_DIR"

echo "builds done!"

if [[ ${1:-} == "--install" ]]; then
  echo ""
  "$OUTPUT_DIR/install.sh"
fi
