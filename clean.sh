#!/usr/bin/env bash

set -e

BUILD_DIR="$HOME/projekts/K1NG-Driver"

# cleanup stuff
echo "cleanup!"
cd $BUILD_DIR
make clean
rm -rf build
rm -rf output

echo "cleanup done!"
