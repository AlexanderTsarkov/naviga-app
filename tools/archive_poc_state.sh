#!/usr/bin/env bash
# Архивирует текущее состояние POC: naviga_app, docs, tools, firmware-release,
# README и скрипты. Для meshtastic-* создаёт MANIFEST (URL + коммит) без копирования репо.

set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCHIVE_BASE="${REPO_ROOT}/archive"
DATE_TAG=$(date +%Y%m%d)
ARCHIVE_DIR="${ARCHIVE_BASE}/poc-${DATE_TAG}"

echo "=== Архивирование состояния POC ==="
echo "Корень репо: ${REPO_ROOT}"
echo "Архив:       ${ARCHIVE_DIR}"
echo ""

mkdir -p "${ARCHIVE_DIR}"
cd "${REPO_ROOT}"

# Копируем приложение (без build/cache, чтобы архив был компактнее)
echo "1. Копирование naviga_app..."
rsync -a --exclude='.dart_tool' --exclude='build' --exclude='*.iml' \
  naviga_app/ "${ARCHIVE_DIR}/naviga_app/" 2>/dev/null || \
  cp -R naviga_app "${ARCHIVE_DIR}/"

# Копируем docs
echo "2. Копирование docs..."
cp -R docs "${ARCHIVE_DIR}/"

# Копируем tools (без этого скрипта в архиве — можно оставить для справки)
echo "3. Копирование tools..."
mkdir -p "${ARCHIVE_DIR}/tools"
for f in tools/*; do
  [ -e "$f" ] && cp -R "$f" "${ARCHIVE_DIR}/tools/"
done

# Копируем firmware-release
if [ -d firmware-release ]; then
  echo "4. Копирование firmware-release..."
  cp -R firmware-release "${ARCHIVE_DIR}/"
else
  echo "4. firmware-release отсутствует — пропуск"
fi

# Копируем README и скрипты из корня
echo "5. Копирование README и скриптов..."
[ -f README.md ] && cp README.md "${ARCHIVE_DIR}/"
[ -f README_GIT_SETUP.md ] && cp README_GIT_SETUP.md "${ARCHIVE_DIR}/"
[ -f setup_naviga_branch.sh ] && cp setup_naviga_branch.sh "${ARCHIVE_DIR}/"

# MANIFEST для meshtastic-* (URL и коммит — чтобы при необходимости заново клонировать)
echo "6. Создание MANIFEST для meshtastic-* репо..."
MANIFEST="${ARCHIVE_DIR}/MANIFEST-meshtastic-repos.txt"
{
  echo "Архив POC: ${DATE_TAG}"
  echo "Создан: $(date -Iseconds 2>/dev/null || date)"
  echo ""
  echo "Репозитории meshtastic-* не копируются в архив (слишком большие)."
  echo "Ниже — remote URL и коммит на момент архивации для повторного клонирования."
  echo ""
} > "${MANIFEST}"

for dir in meshtastic-android meshtastic-ios meshtastic-firmware; do
  if [ -d "${REPO_ROOT}/${dir}/.git" ]; then
    echo "--- ${dir} ---" >> "${MANIFEST}"
    (cd "${REPO_ROOT}/${dir}" && git remote get-url origin 2>/dev/null || echo "origin: not set") >> "${MANIFEST}"
    (cd "${REPO_ROOT}/${dir}" && echo "branch: $(git rev-parse --abbrev-ref HEAD 2>/dev/null)") >> "${MANIFEST}"
    (cd "${REPO_ROOT}/${dir}" && echo "commit: $(git rev-parse HEAD 2>/dev/null)") >> "${MANIFEST}"
    echo "" >> "${MANIFEST}"
  else
    echo "--- ${dir} --- (не git-репо, пропуск)" >> "${MANIFEST}"
  fi
done

echo ""
echo "=== Готово ==="
echo "Архив: ${ARCHIVE_DIR}"
echo "MANIFEST: ${MANIFEST}"
echo ""
echo "Чтобы восстановить удалённые файлы docs из git перед следующим архивом:"
echo "  git restore docs/"
