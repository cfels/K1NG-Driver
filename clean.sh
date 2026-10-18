#!/usr/bin/env bash

set -e

BUILD_DIR="$HOME/projects/K1NG-Driver"

# cleanup stuff
echo "cleanup!"
cd $BUILD_DIR
make clean

echo "cleanup done!"
