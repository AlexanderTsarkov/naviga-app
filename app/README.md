# Naviga — мобильное приложение

Flutter-приложение проекта Naviga (Android-first). Каркас Mobile v1 с базовой навигацией и feature-структурой.

## Запуск (Android)

```bash
cd app
flutter pub get
flutter run
```

## Анализ и тесты

```bash
cd app
flutter analyze
flutter test
```

## Notes

- **minSdkVersion = 28** (Android 9 / Samsung S8 compatible).
- Для будущего BLE-сканирования Android потребует Location permissions — это будет добавлено позже.

## Структура

- `lib/app/` — app shell, routing, state.
- `lib/features/` — feature-based экраны.
- `lib/shared/` — общие виджеты и утилиты.

Документация по приложению: `docs/mobile/`.
