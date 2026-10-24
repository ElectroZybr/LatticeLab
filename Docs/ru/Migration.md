# План перевода плагинов на новую модель (завершён)

Дальнейшая разработка — [Roadmap.md](Roadmap.md).

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

В host-режиме SIGINT останавливает host-сервис на ближайшем `idle` (исправлено после миграции).

## Этап 3. Графика

Решения: один бэкенд WGPU без промежуточного слоя `GPU`; GPU-ресурсы — RAII-объекты внутри
компонентов, учёт живых ресурсов — реестр устройства (действие `resources`); в дереве — компоненты.

- [x] **GPU** — удалён; типы (`BufferDesc`, `TextureFormat`, `Rect`, `NativeWindow`…) переехали в `WGPU`
- [x] **Vulkan** — удалён (неполный, не собирался); при необходимости вернётся вариантом пакета
- [x] **WGPU** — реализация и тесты перенесены в `Packages/WGPU`; классы (`Instance`, `Device`, `Buffer`, `Shader`, `Pipeline`, `BindingSet`, `Surface`, `CommandList`, `ResourceRegistry`) — обычные RAII-объекты в `WGPU.core`; `WGPU` — композиция с общим `Device`, `WGPU.tests` — тесты реализации
- [x] **Graphics** — пакет: `compose.cpp` собирает ветку рендера (слот окна с вводом, surface, viewport с камерой и слотом контроллера, `Render`); классы обычные, в `Graphics.core`
- [x] **Shell** — `compose.cpp`: кейбинды, вложенная ветка `Graphics`, `WindowSystem` как host-сервис (`Shell = ["on", "host"]`); `FrameAPI` разрывает цикл Shell ↔ Graphics
- [x] Проверка: полный `lattice.toml` — окно, рендер, 13 кейбиндов, `resources` показывает живые ресурсы, выход через `quit` с кодом 0

## Этап 4. Уборка

- [x] Из ядра удалены `Component`, `SubsystemAPI`, `Model`, `NodeConfigure` и механизм `configure`/`invalidate` в `maintain`; `BlueprintRegister`, дескрипторы (`Desc`), `NodeBuild::add/share/addImpls/ancestor`, `Children<T>`, `Slot::choice<Impl>()`; lifecycle только через `onRetire`/`readyToDestroy` композиции (методы `retire()`/`readyToDestroy()` больше не подхватываются неявно)
- [x] Удалены `MountBuild::addLocal`/`addShare`, `NodeFactory::addLocal`/`addShare`/`resource`/`reference`, `NodeKind::Mount`/`SharedMount` и ссылки реестра; `mount<T>()` остаётся
- [x] Загрузчик принимает только `compose`; `plugin_register` удалён. `NodeKind::Component` переименован в `NodeKind::Object`
- [x] Корень Runtime — обычная папка с глобальными действиями (`quit`, `add`, `del`, ...), без класса с конструктором от `NodeBuild`
- [x] Тесты ядра переписаны на `emplace`/композиции; тесты удалённых механизмов удалены (198 тестов)
- [x] Тесты Serialization: `NodeBuild::focus` вместо `nodes.configure(root)`
- [x] Codegen удалён (`Tools/Generators` в ядре, подключение в корневом `xmake.lua`)
- [x] `composition.cpp` → `compose.cpp` в примерах ядра
- [x] Удалены пустые `Packages/Compute`, `Packages/ReactiveMD`
- [x] Раздел «Статус миграции» убран из AGENTS.md, `Docs/ru` и README пакетов обновлены

Не сделано: `ForceFields` по-прежнему закомментирован (см. этап 1).

## Структура дерева

- [x] `Serializer` размещает StdIo в своей ветке (`StdIo/Serializer:config`); пакет `Serialization` удалён, код — модуль `Serialization.core`
- [x] `ActionRouter` создаёт Shell в своей ветке; пакет `Actions` удалён, код — модуль `Actions.core`
- [x] Устройство WGPU называется по адаптеру (`Device::NVIDIA GeForce …`)
- [x] Вьюпорт — узел своего объекта (`TestViewport:viewport`) с камерой и слотом контроллера; вьюпортов может быть несколько
- [x] Ядро: `construct` на ветке из `branch()`, запрос `owner<T>()`

## Структура дерева

- [x] `Serializer` размещает StdIo в своей ветке; пакет `Serialization` удалён, код — модуль `Serialization.core`
- [x] `ActionRouter` создаёт Shell в своей ветке; пакет `Actions` удалён, код — модуль `Actions.core`
- [x] Устройство WGPU называется по адаптеру
- [x] Вьюпорт — узел своего объекта с камерой и слотом контроллера; вьюпортов может быть несколько
- [x] Ядро: `construct` на ветке из `branch()`, запрос `owner<T>()`

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

## Переиспользуемые инструменты

- [x] Logger, SysInfo и BmRunner вынесены в `../Lattice/Libs` с отдельными xmake-пакетами (`lattice-logger`, `lattice-sysinfo`, `lattice-bmrunner`); общие header-only утилиты — `lattice-tools`. WGPU.core использует отдельный Logger, Bench.core — отдельный BmRunner.

- [x] Shared-пакеты Logger, SysInfo и BmRunner экспортируют `rpathdirs`: прямой запуск LatticeLab и загрузка Bench не зависят от `LD_LIBRARY_PATH`, который добавляет `xmake run`.

## Пользовательский слой конфигурации

- [x] Serialization сохраняет все явные переопределения по дереву; ядро уведомляет о set/reset/снятии параметров без хранения исходных значений. CLI использует явный set и поддерживает reset; временный ввод Actions остаётся через raw-setter.

- [x] Компактный пользовательский слой: вложенные ключи дерева и параметры без `children`/`params`/`blueprint`/`name`; типы в `Тип:имя` для композиций и неоднозначных детей. Обычные YAML-ключи сохраняются без кавычек, текущий пользовательский файл переведён с сохранением значений.

- [x] Восстановлены WASD/QE и оси мыши для FreeCamera: глобальные экспорты управления, axis/axis2 в загрузчике привязок; CLI показывает статус конфига и использует пользовательскую тему для сообщений об ошибках.

- [x] Пользовательский слой по умолчанию находится в `User/runtime.yaml`; отсутствие файла при первом запуске допустимо, `save` создаёт каталог. Личные настройки перенесены из `Config/`.

- [x] Сервис — роль существующего объекта: состояние выполнения хранится в его lifecycle, дочерние узлы ServiceRunner удалены. В дереве сервис отмечен `[service]`; WindowSystem больше не имеет имени `loop`.

- [x] `NodeBuild::construct<T>` размещает обычный объект прямо в корне композиции. CLI, Shell и ClassicMD объявляют сервис на корне, без промежуточных узлов cli/WindowSystem/loop. Параметры темы CLI находятся непосредственно в ветке CLI.

- CLI: исполнение команд и чтение runtime перенесены в host через lifecycle-hook `onMaintain`. Worker обслуживает терминалы, запросы и ответы адресуются ID сессии; удаление терминала отменяет очередь, остановка не ждёт host.
