#!/usr/bin/env bash

set -e

# VARS
OUTPUT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="$HOME/.local/bin"
INSTALL_DIR="$BIN_DIR/k1ng-driver"
APPLICATIONS_DIR="$HOME/.local/share/applications"
ICONS_DIR="$HOME/.local/share/icons/hicolor/512x512/apps"
DESKTOP_FILE="com.moxiu.k1ng.desktop"
ICON_FILE="logo.png"
OLD_INSTALL_DIR="/opt/k1ng-driver"

# check output
if [[ ! -f "$OUTPUT_DIR/k1ngui" || ! -f "$OUTPUT_DIR/libm3shapes.so" || ! -f "$OUTPUT_DIR/driver_bin/k1ng_driver" ]]; then
  echo "build files not found"
  echo "run this installer from the output made by build.sh"
  exit 1
fi

if [[ ! -f "$OUTPUT_DIR/scripts/applyrules.sh" || ! -f "$OUTPUT_DIR/scripts/run.sh" ]]; then
  echo "scripts not found"
  echo "run build.sh again"
  exit 1
fi

if [[ ! -f "$OUTPUT_DIR/$DESKTOP_FILE" || ! -f "$OUTPUT_DIR/$ICON_FILE" ]]; then
  echo "desktop shortcut files not found"
  echo "run build.sh again"
  exit 1
fi

if [[ $EUID -eq 0 ]]; then
  echo "run this installer without sudo"
  exit 1
fi

# apply rule
bash "$OUTPUT_DIR/scripts/applyrules.sh"

# setup dirs
rm -rf "$INSTALL_DIR"
mkdir -p "$INSTALL_DIR/driver_bin"
mkdir -p "$INSTALL_DIR/scripts"
mkdir -p "$BIN_DIR"
mkdir -p "$APPLICATIONS_DIR"
mkdir -p "$ICONS_DIR"

# install files
cp "$OUTPUT_DIR/k1ngui" "$INSTALL_DIR"
cp "$OUTPUT_DIR/libm3shapes.so" "$INSTALL_DIR"
cp "$OUTPUT_DIR/driver_bin/k1ng_driver" "$INSTALL_DIR/driver_bin"
cp "$OUTPUT_DIR/scripts/applyrules.sh" "$INSTALL_DIR/scripts"
cp "$OUTPUT_DIR/scripts/run.sh" "$BIN_DIR/k1ngui"
cp "$OUTPUT_DIR/$DESKTOP_FILE" "$APPLICATIONS_DIR"
cp "$OUTPUT_DIR/$ICON_FILE" "$INSTALL_DIR"
cp "$OUTPUT_DIR/$ICON_FILE" "$ICONS_DIR/com.moxiu.k1ng.png"
sed -i "s|^Exec=.*|Exec=$BIN_DIR/k1ngui|" "$APPLICATIONS_DIR/$DESKTOP_FILE"
sed -i "s|^Icon=.*|Icon=$INSTALL_DIR/logo.png|" "$APPLICATIONS_DIR/$DESKTOP_FILE"

chmod +x "$INSTALL_DIR/k1ngui"
chmod +x "$INSTALL_DIR/libm3shapes.so"
chmod +x "$INSTALL_DIR/driver_bin/k1ng_driver"
chmod +x "$INSTALL_DIR/scripts/applyrules.sh"
chmod +x "$BIN_DIR/k1ngui"

ln -sf "$INSTALL_DIR/driver_bin/k1ng_driver" "$BIN_DIR/k1ng_driver"

# remove old install
if [[ -L /usr/local/bin/k1ngui ]] && [[ $(readlink -f /usr/local/bin/k1ngui) == "$OLD_INSTALL_DIR/k1ngui" ]]; then
  sudo rm -f /usr/local/bin/k1ngui
  sudo rm -f /usr/local/bin/k1ng_driver
  sudo rm -f /usr/local/share/applications/k1ng-driver.desktop
  sudo rm -rf "$OLD_INSTALL_DIR"
fi

# desktop entry
rm -f "$APPLICATIONS_DIR/k1ng-driver.desktop"
rm -f "$APPLICATIONS_DIR/k1ngui.desktop"
rm -f "$ICONS_DIR/k1ngui.png"
chmod 644 "$APPLICATIONS_DIR/$DESKTOP_FILE"
chmod 644 "$INSTALL_DIR/logo.png"
chmod 644 "$ICONS_DIR/com.moxiu.k1ng.png"

if command -v update-desktop-database >/dev/null 2>&1; then
  update-desktop-database "$APPLICATIONS_DIR" >/dev/null 2>&1 || true
fi

if command -v kbuildsycoca6 >/dev/null 2>&1; then
  kbuildsycoca6 --noincremental >/dev/null 2>&1 || true
fi

echo "driver installed!"
echo "run it with: $BIN_DIR/k1ngui"
