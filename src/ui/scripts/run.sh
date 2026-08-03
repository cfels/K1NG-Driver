#!/usr/bin/env bash

set -e

# VARS
BIN_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
K1NGUI="$BIN_DIR/k1ng-driver/k1ngui"

if [[ ! -x $K1NGUI ]]; then
  echo "k1ngui not found at $K1NGUI"
  exit 1
fi

if [[ ! -t 0 ]]; then
  if command -v kitty >/dev/null 2>&1; then
    exec kitty --title "K1NG Driver" "$0" "$@"
  elif command -v konsole >/dev/null 2>&1; then
    exec konsole -e "$0" "$@"
  elif command -v foot >/dev/null 2>&1; then
    exec foot -T "K1NG Driver" "$0" "$@"
  elif command -v alacritty >/dev/null 2>&1; then
    exec alacritty --title "K1NG Driver" -e "$0" "$@"
  elif command -v wezterm >/dev/null 2>&1; then
    exec wezterm start -- "$0" "$@"
  elif command -v xterm >/dev/null 2>&1; then
    exec xterm -T "K1NG Driver" -e "$0" "$@"
  fi

  echo "terminal not found"
  exit 1
fi

if command -v doas >/dev/null 2>&1; then
  ESCALATOR="doas"
elif command -v sudo >/dev/null 2>&1; then
  ESCALATOR="sudo"
else
  echo "sudo or doas not found"
  exit 1
fi

echo "running $K1NGUI with $ESCALATOR"

if ! "$ESCALATOR" env \
  "DISPLAY=${DISPLAY:-}" \
  "XAUTHORITY=${XAUTHORITY:-}" \
  "DBUS_SESSION_BUS_ADDRESS=${DBUS_SESSION_BUS_ADDRESS:-}" \
  "QT_QPA_PLATFORM=xcb" \
  "$K1NGUI" "$@"; then
  echo ""
  echo "k1ngui failed to start, press enter to close"
  read -r
  exit 1
fi
