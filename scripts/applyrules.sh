#!/usr/bin/env bash

set -e

# VARS
RULE_FILE="99-k1ng-driver.rules"
RULE='SUBSYSTEM=="hidraw", ATTRS{idVendor}=="3554", ATTRS{idProduct}=="f54f", MODE="0666"'
NIX_CONFIG="/etc/nixos/configuration.nix"
NIX_MODULE="/etc/nixos/k1ng-driver.nix"
NIX_BACKUP="/etc/nixos/configuration.nix.k1ng-driver.backup"

# detect os
if [[ ! -f /etc/os-release ]]; then
  echo "couldnt detect os"
  exit 1
fi

source /etc/os-release

# check rules
RULE_FOUND=false

for CHECK_DIR in /etc/udev/rules.d /run/udev/rules.d /usr/lib/udev/rules.d /usr/local/lib/udev/rules.d; do
  if [[ -d $CHECK_DIR ]] && grep -RqsF "$RULE" "$CHECK_DIR"; then
    RULE_FOUND=true
  fi
done

# nixos rules
if [[ $ID == "nixos" ]]; then
  if [[ ! -f $NIX_CONFIG ]]; then
    echo "$NIX_CONFIG not found"
    exit 1
  fi

  NIX_RULE_FOUND=false

  if grep -qsF "$RULE" "$NIX_CONFIG"; then
    NIX_RULE_FOUND=true
  fi

  if [[ $RULE_FOUND == true && $NIX_RULE_FOUND == true ]]; then
    echo "rule already applied!"
    exit 0
  fi

  if [[ $EUID -ne 0 ]]; then
    if command -v sudo >/dev/null 2>&1; then
      exec sudo "$0" "$@"
    fi

    echo "run this script as root"
    exit 1
  fi

  if ! command -v nixos-rebuild >/dev/null 2>&1; then
    echo "nixos-rebuild not found"
    exit 1
  fi

  if grep -qsF './k1ng-driver.nix' "$NIX_CONFIG"; then
    sed -i --follow-symlinks 's|[[:space:]]*\./k1ng-driver\.nix||' "$NIX_CONFIG"
    rm -f "$NIX_MODULE"
  fi

  if [[ $NIX_RULE_FOUND == false ]]; then
    cp "$NIX_CONFIG" "$NIX_BACKUP"

    if grep -qs '^[[:space:]]*services\.udev\.extraRules[[:space:]]*=' "$NIX_CONFIG"; then
      RULES_START=$(grep -n '^[[:space:]]*services\.udev\.extraRules[[:space:]]*=' "$NIX_CONFIG" | head -n 1 | cut -d: -f1)
      RULES_END=$(awk -v START="$RULES_START" "NR >= START && /'';/ { print NR; exit }" "$NIX_CONFIG")

      if [[ -z $RULES_END || $RULES_END == "$RULES_START" ]]; then
        echo "couldnt add rule to services.udev.extraRules"
        exit 1
      fi

      sed -i --follow-symlinks "${RULES_END}i\\    $RULE" "$NIX_CONFIG"
    else
      LAST_LINE=$(grep -n '^[[:space:]]*}[[:space:]]*$' "$NIX_CONFIG" | tail -n 1 | cut -d: -f1)

      if [[ -z $LAST_LINE ]]; then
        echo "couldnt find where to add the nixos rule"
        exit 1
      fi

      sed -i --follow-symlinks "${LAST_LINE}i\\
  services.udev.extraRules = ''\\
    $RULE\\
  '';" "$NIX_CONFIG"
    fi
  fi

  echo "rebuilding nixos config"

  if ! nixos-rebuild switch; then
    if [[ $NIX_RULE_FOUND == false && -f $NIX_BACKUP ]]; then
      cp "$NIX_BACKUP" "$NIX_CONFIG"
      echo "rebuild failed, configuration.nix was restored"
    fi

    exit 1
  fi

  echo "rule applied!"
  exit 0
fi

if [[ $RULE_FOUND == true ]]; then
  echo "rule already applied!"
  exit 0
fi

if [[ $EUID -ne 0 ]]; then
  if command -v sudo >/dev/null 2>&1; then
    exec sudo "$0" "$@"
  fi

  echo "run this script as root"
  exit 1
fi

if ! command -v udevadm >/dev/null 2>&1; then
  echo "udevadm not found"
  exit 1
fi

# apply rule
RULES_DIR="/etc/udev/rules.d"
mkdir -p "$RULES_DIR"
printf '%s\n' "$RULE" > "$RULES_DIR/$RULE_FILE"
chmod 644 "$RULES_DIR/$RULE_FILE"

udevadm control --reload-rules
udevadm trigger --subsystem-match=hidraw --action=change

echo "rule applied!"
