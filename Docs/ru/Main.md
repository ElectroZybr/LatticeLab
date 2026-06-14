> Язык: RU
> Статус EN: создан каркас
# Документация LatticeLab

Этот раздел нужен как рабочая точка входа в проект: что это за репозиторий, из каких частей он состоит, как его собирать и где искать нужный код.

Если нужна краткая презентация проекта, см. [Описание проекта](README.ru.md).
Если нужен обзор архитектуры проекта и роли движка `LATTICE`, см. [Архитектура](./articles/Architecture.md).
## Обзор

LatticeLab — desktop-приложение для интерактивного моделирования материи на движке `LATTICE`.

В репозитории есть несколько основных частей:

- `App/` — пользовательский интерфейс, окна, viewport и glue-код приложения.
- `Lattice/` — физическая модель, данные сцены, логика симуляции.
- `Rendering/` — рендеринг, GPU-пайплайны, слои отрисовки, rendering benchmarks и тесты.
- `Docs/` — документация проекта.
- `assets/` — ресурсы приложения.
- `demo/` — демонстрационные сцены и медиа.

## Навигация

[Главная](Docs/ru/Main.md)
├── [Архитектура](./articles/Architecture.md)
├── [Приложение](./articles/App.md)
├── [Сборка](./articles/Build.md)
├── [Поток Данных](./articles/DataFlow.md)
├── [GUI](./articles/GUI.md)
├── [Термины](./articles/Glossary.md)
├── [Локализация](./articles/Localization.md)
├── [Движок Lattice](./articles/Lattice.md)
├── [Рендер](./articles/Rendering.md)
├── [Тесты](./articles/Tests.md)
├── [WGPU](./articles/WGPU.md)
├── [CLI](./articles/CLI.md)
└── [Benchmarks](./articles/Benchmarks.md)

## Код

Если нужен быстрый вход сразу в проектные точки кода:

- [main.cpp](../../main.cpp)
- [App/Application.cpp](../../App/Application.cpp)
- [App/viewport/SceneViewport.cpp](../../App/viewport/SceneViewport.cpp)
- [App/viewport/SimulationSceneSource.h](../../App/viewport/SimulationSceneSource.h)
- [Lattice/Engine/Simulation.h](../../Lattice/Engine/Simulation.h)
- [Lattice/Engine/World.h](../../Lattice/Engine/World.h)
- [Rendering/Renderer.h](../../Rendering/Renderer.h)
- [Rendering/Renderer.cpp](../../Rendering/Renderer.cpp)
- [Rendering/RenderData.h](../../Rendering/RenderData.h)
- [Rendering/backend/WGPUContext.h](../../Rendering/backend/WGPUContext.h)

## Сборка

Полное руководство по сборке вынесено в отдельную статью: [Сборка](./articles/Build.md).

Коротко:

- используются `CMakePresets`
- основная локальная сборка обычно идет через `debug` или `release`
- benchmarks собираются отдельным preset-ом `bench`

```bash
cmake --preset debug
cmake --build --preset debug
```
