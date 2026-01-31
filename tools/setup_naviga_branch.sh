#!/bin/bash
# Скрипт для настройки ветки Naviga с сохранением изменений
# Запуск из корня репо: ./tools/setup_naviga_branch.sh

set -e  # Остановиться при ошибке

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

echo "=== Настройка ветки Naviga ==="
echo ""

cd meshtastic-firmware

# Проверка текущего состояния
echo "1. Проверка текущего состояния..."
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
echo "   Текущая ветка: $CURRENT_BRANCH"
echo ""

# Проверка изменений
if [ -n "$(git status --porcelain)" ]; then
    echo "2. Обнаружены незакоммиченные изменения:"
    git status --short
    echo ""
    read -p "Сохранить изменения? (y/n): " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        # Создать ветку naviga-custom если её нет
        if ! git show-ref --verify --quiet refs/heads/naviga-custom; then
            echo "   Создание ветки naviga-custom..."
            git checkout -b naviga-custom
        else
            echo "   Переключение на ветку naviga-custom..."
            git checkout naviga-custom
        fi
        
        # Сохранить изменения
        echo "   Сохранение изменений..."
        git add -A
        git commit -m "Naviga: Malaysia region TX power 22 dBm for T-Beam"
        echo "   ✓ Изменения сохранены"
    else
        echo "   Изменения не сохранены. Продолжить? (y/n): "
        read -p "" -n 1 -r
        echo ""
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Прервано пользователем"
            exit 1
        fi
    fi
else
    echo "2. Незакоммиченных изменений не обнаружено"
    # Создать ветку naviga-custom если её нет
    if ! git show-ref --verify --quiet refs/heads/naviga-custom; then
        echo "   Создание ветки naviga-custom..."
        git checkout -b naviga-custom
    else
        echo "   Переключение на ветку naviga-custom..."
        git checkout naviga-custom
    fi
fi

echo ""

# Настройка upstream
echo "3. Настройка upstream..."
if git remote | grep -q "^upstream$"; then
    echo "   Upstream уже настроен"
else
    echo "   Добавление upstream..."
    git remote add upstream https://github.com/meshtastic/firmware.git
    echo "   ✓ Upstream добавлен"
fi

echo ""

# Получение информации о ветках
echo "4. Получение информации о ветках..."
git fetch upstream
echo "   ✓ Информация получена"

echo ""

# Показать доступные версии
echo "5. Доступные версии:"
echo "   Последние теги v2.7:"
git tag | grep "^v2\.7" | tail -5
echo ""
echo "   Последние теги v2.6:"
git tag | grep "^v2\.6" | tail -5

echo ""
echo "=== Настройка завершена ==="
echo ""
echo "Текущая ветка: $(git rev-parse --abbrev-ref HEAD)"
echo ""
echo "Следующие шаги:"
echo "1. Для обновления до v2.7.14 выполните:"
echo "   git checkout -b naviga-update-2.7.14 upstream/v2.7.14"
echo "   git merge naviga-custom"
echo ""
echo "2. Или для обновления до develop (последняя версия):"
echo "   git checkout -b naviga-update-develop upstream/develop"
echo "   git merge naviga-custom"
echo ""
echo "Подробные инструкции в docs/Git_Workflow_For_Fork.md"



