#!/usr/bin/env bash
# Клонирует официальные репозитории Meshtastic в reference/ для чтения кода
# (архитектура, организация, решения). Не трогает существующие meshtastic-* в корне.
# См. docs/REFERENCE_REPOS.md

set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REFERENCE_DIR="${REPO_ROOT}/reference"
DEPTH=1

echo "=== Референсные репо Meshtastic ==="
echo "Папка: ${REFERENCE_DIR}"
echo "Глубина клона: --depth ${DEPTH}"
echo ""

mkdir -p "${REFERENCE_DIR}"
cd "${REFERENCE_DIR}"

clone_if_missing() {
  local dir="$1"
  local url="$2"
  if [ -d "$dir/.git" ]; then
    echo "  $dir уже есть — пропуск"
    return
  fi
  echo "  Клонирование $dir..."
  git clone --depth "$DEPTH" "$url" "$dir"
}

clone_if_missing "meshtastic-android" "https://github.com/meshtastic/Meshtastic-Android.git"
clone_if_missing "meshtastic-ios"     "https://github.com/meshtastic/Meshtastic-Apple.git"
clone_if_missing "meshtastic-firmware" "https://github.com/meshtastic/firmware.git"

echo ""
echo "Готово. Референсный код в: ${REFERENCE_DIR}"
echo "Папка reference/ в .gitignore — в репо Naviga не попадёт."
