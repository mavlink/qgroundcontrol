# Clipper2 в custom-сборке QGroundControl

## Установка и подключение

Библиотека **Clipper2** подключается автоматически при сборке с custom-плагином через **CPM** (C++ Package Manager). Ничего ставить вручную не нужно.

- Конфигурация: `custom/CMakeLists.txt` — добавлен `CPMAddPackage` для репозитория [AngusJohnson/Clipper2](https://github.com/AngusJohnson/Clipper2), тег `2.0.1`, подкаталог `CPP`.
- Сборка: утилиты, примеры и тесты Clipper2 отключены (`CLIPPER2_UTILS/EXAMPLES/TESTS OFF`), собирается только библиотека `Clipper2`.
- Связь с приложением: `Clipper2` добавлен в `CUSTOM_LIBRARIES`; корневой `CMakeLists.txt` линкует его с основным исполняемым файлом (`target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${CUSTOM_LIBRARIES})`).

При первой конфигурации CPM скачает исходники Clipper2 в кэш (по умолчанию `build/cpm_modules` или значение `QGC_CPM_SOURCE_CACHE`).

## Использование в коде (Spray и др.)

- Подключение заголовков:
  - Основной API: `#include <clipper2/clipper.h>`
  - Дополнительно при необходимости: `clipper.offset.h`, `clipper.core.h` и т.д. (путь относительно `Clipper2Lib/include/clipper2`).
- Линковка: целевой таргет уже линкует `Clipper2` через `CUSTOM_LIBRARIES`, отдельно указывать библиотеку не нужно.
- Namespace: `Clipper2Lib` (например, `Clipper2Lib::PathsD`, `Clipper2Lib::Intersect()`).

Пример для обрезки полигонов и offset (буфер препятствий):

```cpp
#include <clipper2/clipper.h>

// Полигоны в виде PathsD (double): каждый путь — замкнутый контур
Clipper2Lib::PathsD subject = { ... };  // полигон поля
Clipper2Lib::PathsD clip    = { ... };  // полигоны препятствий

Clipper2Lib::PathsD result = Clipper2Lib::Intersect(subject, clip, Clipper2Lib::FillRule::NonZero);

// Offset: внешнее расширение полигона на margin (в тех же единицах, что и координаты)
double margin = 1.5;
Clipper2Lib::PathsD expanded = Clipper2Lib::InflatePaths(clip, margin, Clipper2Lib::JoinType::Round, Clipper2Lib::EndType::Polygon);
```

Координаты в Clipper2 задаются в произвольных единицах (например, метры в NED или локальной сетке). В Spray удобно переводить вершины полигонов/линий в локальную систему (например, от центра поля), вызывать Clipper2, затем обратно в координаты миссии.

## Полезные ссылки

- [Обзор Clipper2](https://www.angusj.com/clipper2/Docs/Overview.htm)
- [Документация API](https://www.angusj.com/clipper2/Docs/Overview.htm)
- [Репозиторий](https://github.com/AngusJohnson/Clipper2)
