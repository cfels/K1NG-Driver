#!/usr/bin/env bash

make clean

echo "build driver"
nix-shell --run "make clean && make"
cp k1ng_driver src/ui/driver_bin

echo "building UI"
nix-shell --run "mkdir build && cd build && make clean && cmake .. && make -j$(nproc)"

echo "builds done!"
