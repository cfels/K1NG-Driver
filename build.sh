#!/usr/bin/env bash

set -e

echo "cleanup!"
make clean
rm -rf build
echo ""

echo "build driver"
nix-shell --run "make clean && make"
cp k1ng_driver src/ui/driver_bin
echo ""

echo "building UI"
nix-shell --run "mkdir -p build && cd build && cmake .. && make -j$(nproc)"
ln -sf build/compile_commands.json compile_commands.json
echo ""

echo "builds done!"
