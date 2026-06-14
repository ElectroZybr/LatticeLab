> Язык: RU
> Canonical: да
> Статус EN: не создан
# Модуль App

`App/` — это прикладной слой LatticeLab. Он отвечает за все, что связано с пользовательским приложением: окно, UI, viewport, обработку ввода и связывание симуляции с рендером.

## Роль

- создание и жизненный цикл окна приложения
- пользовательский интерфейс
- viewport и взаимодействие с камерой
- обработка клавиатуры, мыши и событий окна
- координация между `Lattice/` и `Rendering/`

## Границы

- физическая модель симуляции
- детали GPU-пайплайнов и низкоуровневого draw
- логика, которая должна переиспользоваться вне UI

## Вход

- `main.cpp`
- `Application`
- `SceneViewport`
- `Interface`
- `ToolsManager`

## Точка Входа

Точка входа приложения минимальна:

```text
main()
  -> Application application
  -> application.run()
```

Дальше основная сборка систем происходит внутри `Application::run()`.

На этом этапе приложение:

- загружает пользовательские настройки
- создает окно
- создает `Lattice::Simulation`
- создает `SceneViewport`
- через рендер внутри viewport поднимает графический контекст
- создает UI через `Interface`
- подключает input/event/tool/debug/capture подсистемы
- запускает главный цикл

## Код

- [main.cpp](../../../main.cpp)
- [App/Application.h](../../../App/Application.h)
- [App/Application.cpp](../../../App/Application.cpp)
- [App/viewport/SceneViewport.h](../../../App/viewport/SceneViewport.h)
- [App/viewport/SceneViewport.cpp](../../../App/viewport/SceneViewport.cpp)
- [App/viewport/SimulationSceneSource.h](../../../App/viewport/SimulationSceneSource.h)
- [App/AppActions.cpp](../../../App/AppActions.cpp)
- [App/interaction/ToolsManager.h](../../../App/interaction/ToolsManager.h)
- [App/GUI/interface/interface.h](../../../App/GUI/interface/interface.h)
- [App/GUI/io/manager/EventManager.cpp](../../../App/GUI/io/manager/EventManager.cpp)

## Основные Подсистемы

### `Application`

Это верхний управляющий слой приложения.

Он отвечает за:

- начальную инициализацию
- wiring между модулями
- главный цикл
- сохранение пользовательских настроек при завершении

### `SceneViewport`

`SceneViewport` — это мост между приложением, сценой и рендером.

Обычно он отвечает за:

- выбор рендерера (`2D` или `3D`)
- синхронизацию сцены с render-данными
- вызов `renderFrame(...)`
- работу с камерой и экранным размером

### `Interface`

`Interface` — это UI-слой приложения.

Здесь находятся:

- панели интерфейса
- пользовательские настройки
- визуальное управление сценой
- состояние UI, которое влияет на поведение приложения

Подробно GUI-слой описан в [GUI.md](./GUI.md).
Локализация интерфейса описана в [Localization.md](./Localization.md).

### `ToolsManager`

`ToolsManager` управляет интерактивными инструментами.

Типичные примеры:

- выделение
- добавление/удаление атомов
- линейка
- lasso
- frame tool
- thermal tool

### `EventManager`

Отвечает за отслеживание и маршрутизацию событий окна и ввода.

### `CaptureController`

Отвечает за запись кадров, экспорт и связанные с этим настройки.

### `WindowController`

Инкапсулирует работу с состоянием окна:

- размер
- позиция
- fullscreen/windowed режим
- snapshot для сохранения настроек

## Цикл Приложения

В грубом виде основной цикл устроен так:

```text
while application is running:
  poll events
  update input state
  update capture state
  update simulation
  render frame
  refresh debug data
```

Внутри `Application::run()` отдельно копятся интервалы для:

- физики
- рендера
- логов и debug-обновлений

Это позволяет не смешивать все обновления в один одинаковый шаг.

## Связь Модулей

На уровне `App` связь модулей обычно выглядит так:

```text
Application
  -> Simulation
  -> SceneViewport
       -> Renderer
  -> Interface
  -> ToolsManager
  -> EventManager
  -> CaptureController
```

То есть `App` собирает систему целиком, но не должен тащить в себя внутреннюю логику `Lattice` или `Rendering`.

## Модель

`App/` не должен "знать физику мира" и не должен превращаться в рендер-движок. Его задача — собрать приложение как продукт и дать пользователю удобную точку управления сценой.

## Ссылки

- [Architecture.md](./Architecture.md)
- [GUI.md](./GUI.md)
- [Lattice.md](./Lattice.md)
- [Rendering.md](./Rendering.md)
