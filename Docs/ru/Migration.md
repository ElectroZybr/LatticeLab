# План перевода плагинов на новую модель

Цель: реализация каждого пакета — обычный C++ код, всё знание о Lattice — в `compose.cpp`.
Правила модели — в [AGENTS.md](../../AGENTS.md).

Порядок — снизу вверх по зависимостям: к моменту перевода потребителя его зависимости
уже обычные C++ классы. Один пакет — один коммит.

## Критерий готовности пакета

- [ ] Цели сборки `<Name>.core` (реализация, без линковки runtime), `<Name>` (`compose.cpp`), `<Name>.tests`
      (библиотеки без ветки и без зависимости от runtime — в `Libs/<Name>`: цели `<Name>` и `<Name>.tests`)
- [ ] Вся работа с Lattice — только в `compose.cpp` (`extern "C" void compose(NodeBuild root)`)
- [ ] В реализации нет Lattice, кроме header-only (`Lattice::Value`, `Lattice::Exception`)
- [ ] Нет наследования от `Component`, `SubsystemAPI`, `Model`; нет `NodeBuild`/`NodeConfigure` в классах
- [ ] Нет `plugin_register`, `BlueprintRegister` и codegen (`codegen.json`, `lattice.plugin_codegen`)
- [ ] Lifecycle через `onRetire` / `readyToDestroy` / `service` в композиции
- [ ] Тесты реализации работают без runtime
- [ ] README пакета описывает дерево ветки и использование вне runtime

## Готово

- [x] StdIo
- [x] Serialization

## Этап 1. Симуляция

- [x] **StdData** — `Libs/StdData`, header-only; `SoA` реализует чистый `Lattice::Table`; удалены закомментированные загрузчики и устаревшие тесты
- [x] **ParticleDynamics** — `Libs/ParticleDynamics`; контракты без `Component`, с виртуальными деструкторами; `SpatialGrid` без configure
- [x] **Integrators** — `Libs/Integrators` (static); `compose.cpp` удалён — своей ветки нет
- [ ] **ForceFields** — не переводился: код целиком закомментирован и ссылается на старый движок (`AtomData::Type`, `getProps`); это новая реализация, а не миграция
- [x] **ClassicMD** — `compose.cpp` собирает ветку; цикл — обычный `SimulationLoop` + `service()`; `SimulationService : Model` и подмена `BlueprintOps` удалены
- [x] Проверка: `ClassicMD.Universe1 = "on"` собирает дерево, сервис стартует и останавливается по SIGINT

Не перенесено (закомментировано и раньше не собиралось): `Libs/ParticleDynamics/ParticleDynamics/NeighborList.*`,
`Libs/Integrators/Integrators/KDK.cpp`, `Libs/Integrators/Integrators/StepOps.hpp`.

## Этап 2. Управление

Независим от этапа 1, можно вести параллельно.

- [x] **Actions** — пакет: `compose.cpp` публикует общий `ActionRouter` (`emplaceShared`); `InputAPI` — обычный интерфейс; владельцы устройств вызывают `registerInput`/`unregisterInput`
- [x] **CLI** — `CLI` и `Terminal` обычные классы; команды/тема/параметры терминала и `service()` в `compose.cpp`
- [x] **LatticeBench** — `Benchmarks` обычный класс; результаты хранятся в объекте, `view <name>` вместо узлов-таблиц
- [x] Проверка: CLI в runtime выполняет `tree`/`cd`/`ls`/`help`/`quit`; полный `lattice.toml` запускается

В `lattice.toml` добавлено `Actions = "on"`; Shell получает роутер через `mount<ActionRouter>()`.

Известное ограничение ядра (было и раньше): в host-режиме SIGINT не останавливает приложение —
`Runtime` проверяет флаг прерывания только без host-сервиса. Выход: `quit` / закрытие окна.

## Этап 3. Графика

Решения: один бэкенд WGPU без промежуточного слоя `GPU`; GPU-ресурсы — RAII-объекты внутри
компонентов, учёт живых ресурсов — реестр устройства (действие `resources`); в дереве — компоненты.

- [x] **GPU** — удалён; типы (`BufferDesc`, `TextureFormat`, `Rect`, `NativeWindow`…) переехали в `WGPU`
- [x] **Vulkan** — удалён (неполный, не собирался); при необходимости вернётся вариантом пакета
- [x] **WGPU** — классы в `Libs/WGPU` (`Instance`, `Device`, `Buffer`, `Shader`, `Pipeline`, `BindingSet`, `Surface`, `CommandList`, `ResourceRegistry`) — обычные RAII-объекты; пакет `Packages/WGPU` — только `compose.cpp` с общим `Device`
- [x] **Graphics** — пакет: `compose.cpp` собирает ветку рендера (слот окна с вводом, surface, viewport с камерой и слотом контроллера, `Render`); классы обычные, в `Graphics.core`
- [x] **Shell** — `compose.cpp`: кейбинды, вложенная ветка `Graphics`, `WindowSystem` как host-сервис (`Shell = ["on", "host"]`); `FrameAPI` разрывает цикл Shell ↔ Graphics
- [x] Проверка: полный `lattice.toml` — окно, рендер, 13 кейбиндов, `resources` показывает живые ресурсы, выход через `quit` с кодом 0

## Этап 4. Уборка

- [ ] Удалить из ядра `../Lattice`: `Component`, `SubsystemAPI`, `Model`, `NodeConfigure`, пакетный `plugin_register`/`BlueprintRegister`
- [ ] Удалить `MountBuild::addLocal`/`addShare`, `NodeFactory::addLocal`/`addShare`, `NodeKind::Mount`/`SharedMount` и их обработку в `NodeOps`/`NodeQuery` (пользователей больше нет), тесты в `Mount_tests.cpp`/`Slot_tests.cpp`
- [ ] Тесты Serialization: заменить `nodes.configure(root).focus<IOSubsystem>()` на `NodeBuild::focus`
- [ ] Удалить codegen (`Tools/Generators/plugin_codegen.lua` и его подключение в корневом `xmake.lua`; пакетов с codegen больше нет)
- [ ] Переименовать `composition.cpp` → `compose.cpp` в примерах ядра (`examples/composition`, `examples/slot`) и `Docs/ru/Kernel/Composition.md`
- [ ] Удалить пустые `Packages/Compute`, `Packages/ReactiveMD` или наполнить
- [ ] Убрать раздел «Статус миграции» из AGENTS.md, обновить `Docs/ru`

## Изменения ядра (`../Lattice`)

- `ObjectBuild::service(run, stop)` + `ServiceRunner`; Runtime ищет сервис в поддереве стартовой ветки
- `Table` — чистый контракт (без `Component` и конструктора с `NodeBuild`); `Lattice::exportTable(obj)`
- `NodeBuild::exports()`; `ObjectBuild::param(name, value&)`, `globalAction`, методы с `ActionContext&`
- `ObjectBuild::action(name, callable)` — действие объекта, реализованное в композиции
- `NodeBuild::compose(package, name)` — вложить ветку другого пакета
- Метаданные пакета в коде: `Lattice::Package lattice_package` (`Kernel/Package.hpp`) вместо `Plugin.toml`; ядро больше не зависит от toml++
- Тесты: `tests/Service_tests.cpp`, `Table_tests.cpp`, `Composition_tests.cpp`

## Открытые решения

1. [x] `Table` — чистый header-only контракт, экспорты в композиции
2. [x] Vulkan — удалён
3. [x] Приложение запускается после каждого этапа
4. [x] Этапы 1–2 — последовательно в одной сессии
