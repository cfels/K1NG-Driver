#!/usr/bin/env bash

set -e

if [[ $# -ne 0 ]]; then
  printf 'usage: %s\n' "$0" >&2
  exit 1
fi

PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

cd "$PROJECT_DIR"

echo "build driver"
nix-shell --run "make clean && make"

echo "builds done!"
