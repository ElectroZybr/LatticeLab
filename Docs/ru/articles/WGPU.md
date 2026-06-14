> Язык: RU
> Статус EN: создан каркас
# WGPU

`WGPU` в проекте используется как графический бэкенд для `Rendering/`. Через него поднимаются `Instance`, `Adapter`, `Device`, `Queue`, `Surface`, depth target, GPU-буферы и render pipeline. Эта роль сосредоточена в `WGPUContext`.

## Код

- [Rendering/backend/WGPUContext.h](../../../Rendering/backend/WGPUContext.h)
- [Rendering/backend/WebGPUImpl.cpp](../../../Rendering/backend/WebGPUImpl.cpp)
- [Rendering/Renderer.cpp](../../../Rendering/Renderer.cpp)
- [Rendering/pipelines/PipelineHelpers.h](../../../Rendering/pipelines/PipelineHelpers.h)
- [Rendering/benchmarks/Fixture.h](../../../Rendering/benchmarks/Fixture.h)
- [App/GUI/interface/interface.cpp](../../../App/GUI/interface/interface.cpp)

## Почему Выбран

`WGPU` был выбран не только как графический API, но и сразу с запасом под compute-направление.

Основные причины:

- единый слой графического бэкенда для нескольких платформ
- современная explicit-модель GPU API
- достаточно низкий уровень контроля без тяжелого внешнего engine
- удобный режим без окна для benchmark-сценариев
- технологический зазор под будущие compute-задачи

Для проекта это означает, что рендерер можно строить как собственную систему со своими слоями, буферами и pipeline state, не привязываясь к чужой scene graph архитектуре.

## Плюсы

- кроссплатформенность: один render path поверх разных нативных бэкендов
- явная модель ресурсов: буферы, bind groups, pipeline layout, pass
- удобен для собственного рендерера без лишней framework-магии
- поддерживает инициализацию без окна, что полезно для benchmarks и технических прогонов
- не закрывает путь к отдельному compute backend в будущем

## Минусы

- экосистема уже, чем у `Vulkan` или крупных game engine
- меньше готового tooling и типовых решений
- не дает полного backend-specific контроля
- behavior и maturity native WebGPU-стека еще нужно иногда проверять на практике

## Ограничения

- `WGPU` не отменяет необходимость держать границу между физикой и GPU-логикой чистой
- не любой низкоуровневый или vendor-specific прием естественно ложится в его модель
- режим без окна удобен для benchmarks, но сам по себе не делает проект готовой серверной GPU-платформой

## Перспектива

Для текущего масштаба LatticeLab это разумный компромисс между переносимостью, контролем и сложностью.

## Связи

- [Входная Точка](Docs/ru/Main.md)
- [Термины](./Glossary.md)
- [Рендер](./Rendering.md)
- [Benchmarks](./Benchmarks.md)
- [Архитектура](./Architecture.md)
