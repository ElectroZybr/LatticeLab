> Язык: RU
> Canonical: да
> Статус EN: не создан
# Документация LatticeLab

Этот раздел нужен как рабочая точка входа в проект: что это за репозиторий, из каких частей он состоит, как его собирать и где искать нужный код.

Если нужна общая презентация проекта, см. [README.ru.md](../../README.ru.md).
Если нужен обзор архитектуры проекта и роли движка `LATTICE`, см. [Architecture.md](./articles/Architecture.md).
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

[README.md](./README.md)
├── [Architecture.md](./articles/Architecture.md)
├── [App.md](./articles/App.md)
├── [Build.md](./articles/Build.md)
├── [DataFlow.md](./articles/DataFlow.md)
├── [GUI.md](./articles/GUI.md)
├── [Glossary.md](./articles/Glossary.md)
├── [Localization.md](./articles/Localization.md)
├── [Lattice.md](./articles/Lattice.md)
├── [Rendering.md](./articles/Rendering.md)
├── [Tests.md](./articles/Tests.md)
├── [WGPU.md](./articles/WGPU.md)
├── [CLI.md](./articles/CLI.md)
└── [Benchmarks.md](./articles/Benchmarks.md)

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

Полное руководство по сборке вынесено в отдельную статью: [Build.md](./articles/Build.md).

Коротко:

- используются `CMakePresets`
- основная локальная сборка обычно идет через `debug` или `release`
- benchmarks собираются отдельным preset-ом `bench`

```bash
cmake --preset debug
cmake --build --preset debug
```
