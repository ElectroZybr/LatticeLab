> Язык: RU
> Canonical: да
> Статус EN: не создан
# Поток Данных

Эта статья описывает основной путь данных в проекте: как состояние симуляции попадает в рендер, где происходит преобразование в draw-формат и через какие слои пользовательское действие возвращается обратно в движок.

## Схема

```text
Simulation
  -> World
  -> syncRendererWithSimulation(...)
  -> RenderData
  -> SceneViewport
  -> BaseRenderer / RendererWGPU
  -> WGPUContext
  -> GPU draw pass
```

Обратное направление выглядит так:

```text
UI / input / tools
  -> AppActions / ToolsManager
  -> Simulation / World
  -> syncRendererWithSimulation(...)
  -> новый кадр
```

## Код

- [App/Application.cpp](../../../App/Application.cpp)
- [App/viewport/SceneViewport.cpp](../../../App/viewport/SceneViewport.cpp)
- [App/viewport/SimulationSceneSource.h](../../../App/viewport/SimulationSceneSource.h)
- [Lattice/Engine/Simulation.cpp](../../../Lattice/Engine/Simulation.cpp)
- [Lattice/Engine/World.cpp](../../../Lattice/Engine/World.cpp)
- [Rendering/BaseRenderer.h](../../../Rendering/BaseRenderer.h)
- [Rendering/RenderData.h](../../../Rendering/RenderData.h)
- [Rendering/Renderer.cpp](../../../Rendering/Renderer.cpp)
- [Rendering/backend/WGPUContext.h](../../../Rendering/backend/WGPUContext.h)

## Прямой Путь

### 1. `Simulation`

`Application` создает `Lattice::Simulation`, добавляет миры и обновляет их в основном цикле через `simulation.updateAll()`.

На этом уровне живет физическое состояние, а не draw-представление.

### 2. `World`

Каждый `World` хранит атомы, связи, `SpatialGrid`, `NeighborList`, `VectorField`, runtime state и параметры расчета.

После шага симуляции именно `World` содержит актуальные данные сцены.

### 3. `syncRendererWithSimulation(...)`

Ключевая граница между движком и рендером находится в `App/viewport/SimulationSceneSource.h`.

Функция `syncRendererWithSimulation(...)`:

- обходит все миры из `Simulation`
- подготавливает `RenderData` для каждого мира
- строит `RenderAtomsView` поверх `AtomStorage`
- пробрасывает bonds и grid через callback-based view
- для активного мира добавляет `RenderVectorFieldView`
- обновляет bounds камеры

Именно здесь физические структуры переводятся в render-friendly формат.

### 4. `RenderData`

`RenderData` не хранит модель мира. Он хранит снимок того, что рендерер должен уметь нарисовать:

- view атомов
- view связей
- view сетки
- view векторного поля
- флаги слоев
- визуальные параметры

Это контракт между `Lattice` и `Rendering`.

### 5. `SceneViewport`

`SceneViewport` связывает приложение и рендерер.

В `renderFrame(...)` он:

- синхронизирует рендерер с `Simulation`
- обновляет UI и камеру
- передает кадр в `CaptureController`
- вызывает `renderer.drawShot(...)`

То есть viewport не считает физику и не делает низкоуровневый GPU-код. Он оркестрирует кадр.

### 6. `BaseRenderer` / `RendererWGPU`

`BaseRenderer` хранит массив `RenderData`.

Конкретная реализация `RendererWGPU` в `drawShot(...)`:

- берет каждый `RenderData`
- вызывает `drawWorldPass(...)`
- обновляет uniform-состояние сцены
- последовательно рисует включенные слои

На этом уровне данные уже считаются render input, а не частью физической модели.

### 7. `WGPUContext`

`WGPUContext` дает доступ к `Device`, `Queue`, `Surface`, depth target и созданию GPU-ресурсов.

`RendererWGPU` использует его для:

- создания encoder/pass
- записи uniform и vertex данных в буферы
- submit команд в очередь

После этого кадр уходит на GPU.

## Обратный Путь

Обратный поток данных начинается не в рендере, а в пользовательских системах.

### UI И Input

Изменения из интерфейса, хоткеев и инструментов проходят через:

- `AppActions`
- `ToolsManager`
- UI panels

Они меняют:

- параметры симуляции
- активные режимы
- настройки рендера
- выбор объектов

### Изменение Состояния

Если действие относится к физике, оно меняет `Simulation` или `World`.

Если действие относится только к визуализации, оно меняет текущий `RenderData` или параметры camera/viewport.

### Следующий Кадр

На следующем `renderFrame(...)` состояние снова проходит через `syncRendererWithSimulation(...)`, и рендерер получает уже обновленную картину мира.

## Почему Так

Такой поток полезен по нескольким причинам:

- физика не зависит от GPU API
- рендерер не работает напрямую с внутренностями `World`
- точка преобразования данных в draw-формат одна и явно локализована
- benchmarks могут использовать рендерер отдельно от полного UI-цикла

Главная архитектурная идея здесь простая: мир считает свое состояние, viewport переводит его в render contract, рендерер только рисует.

## Связи

- [README.md](../README.md)
- [Glossary.md](./Glossary.md)
- [Lattice.md](./Lattice.md)
- [Rendering.md](./Rendering.md)
- [WGPU.md](./WGPU.md)
- [App.md](./App.md)
