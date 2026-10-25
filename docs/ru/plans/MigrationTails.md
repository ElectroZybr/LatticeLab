# Хвосты после миграции

Миграция на модель «реализация + `compose.cpp`» завершена; её проект — `docs/ru/Migration.md` в истории git (до `26618d42`).

## Сделано

- [x] Ядро: plugin → package (`PackageManager`, `PackageCandidate`, `PackageCatalog`, логи, `tests/TestPackages`)
- [x] Shell: `glfw*` → `Glfw*` (классы и файлы); namespace CLI `CLIPlugin` → `Console`
- [x] Host-режим: SIGINT останавливает host-сервис на ближайшем `idle`
- [x] Один курсор окна: параметр `cursor` у `Render` (владелец surface), hit-test по слоям вьюпортов, локальная координата у вьюпорта под курсором
- [x] `cursor` и управление камерой — локальные экспорты; роутер разрешает кейбинды от цели (`setTarget`), цель — контроллер вьюпорта под курсором
