# Структура репозитория Naviga

Документ описывает структуру репозитория после перехода на вариант B (чистый вид для разработки продукта).

## Обзор

Один репозиторий (monorepo), чёткое разделение по частям продукта (вариант B):

| Компонент | Назначение | Git |
|-----------|------------|-----|
| **app/** | Мобильное приложение Naviga (Flutter, iOS + Android) | Часть репо |
| **firmware/** | Прошивка (placeholder / submodule при необходимости) | Часть репо |
| **backend/** | Серверная часть | Часть репо |
| **web/** | Веб-приложение | Часть репо |
| **docs/** | Вся документация; подкаталоги: firmware/, mobile-app/, backend/, web/, design/, adr/ | Часть репо |
| **tools/** | Скрипты сборки, отладки, утилиты | Часть репо |
| **archive/** | Архив POC (poc-YYYYMMDD) | В .gitignore |
| **meshtastic-*** | Референс Meshtastic (архитектура, код) | Локальные клоны, в .gitignore |
| **firmware-release/** | Собранные бинарники прошивки | В .gitignore |

---

## Детали по директориям

### app/ — мобильное приложение Flutter

- **Платформы:** Android, iOS (основные), Linux, macOS, Web, Windows
- **Ключевые части:** `lib/` (meshtastic/, screens/, services/, generated/), proto, android/, ios/
- **Статус:** переход от POC к продукту

### firmware/, backend/, web/

- **firmware/** — место под прошивку (submodule или ссылки; исходники/форк — см. docs/firmware и REFERENCE_REPOS).
- **backend/**, **web/** — серверная часть и веб-приложение (код по мере разработки).

### meshtastic-android / meshtastic-ios / meshtastic-firmware (референс)

- **Назначение:** только как референс — архитектура, код, решения. В репо Naviga не коммитятся (.gitignore).
- Как получить копии — см. **docs/REFERENCE_REPOS.md**. Скрипт настройки ветки прошивки: **tools/setup_naviga_branch.sh**.

### firmware-release/

- Готовые .bin/.factory образы прошивки для устройств (например T-Beam).
- Не коммитятся в git (артефакты сборки).

### tools/

- Скрипты сборки, отладки (например `dual_serial_log.py`, `setup_naviga_branch.sh`, `archive_poc_state.sh`, `setup_reference_repos.sh`).
- Часть репо.

### docs/

- Вся документация проекта; подкаталоги по областям: firmware/, mobile-app/, backend/, web/, design/, adr/.
- Общие документы в корне docs/: CLEAN_SLATE.md, REPO_STRUCTURE.md, REFERENCE_REPOS.md; по прошивке — docs/firmware/ (в т.ч. README_GIT_SETUP.md).

### Корень репо

- `README.md` — описание проекта и структура
- `.gitignore` — референсные репо meshtastic-*, archive/, firmware-release/, reference/

---

## Зависимости между компонентами

- **app** (Flutter) не собирается из meshtastic-*; использует свои proto и код в `lib/`.
- **firmware** — исходники/форк прошивки отдельно (см. docs/firmware, REFERENCE_REPOS); скрипт `tools/setup_naviga_branch.sh` настраивает ветку в клоне meshtastic-firmware.
- **meshtastic-android**, **meshtastic-ios** — только референс, в .gitignore.
