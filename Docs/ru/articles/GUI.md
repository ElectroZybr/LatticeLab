> Язык: RU
> Статус EN: создан каркас
# GUI

`App/GUI/` отвечает за пользовательский интерфейс приложения: `ImGui`-контекст, панели, стили, шрифты, диалоги, ввод и связку UI с состоянием симуляции и viewport.

## Роль

- инициализация `ImGui`
- интеграция `ImGui` с `GLFW` и `WGPU`
- хранение и обновление `UiState`
- отрисовка панелей и оверлеев
- маршрутизация пользовательского ввода на уровень интерфейса

## Границы

- GUI не считает физику
- GUI не должен содержать низкоуровневую GPU-логику рендера
- GUI меняет состояние приложения, но не подменяет собой `Simulation` и `Rendering`

## Структура

Основные части модуля:

- `interface/` — верхний UI-слой и панели
- `io/` — клавиатура, мышь, window events, event manager

На уровне ключевых объектов:

- `Interface` — корневой объект GUI
- `UiState` — общее состояние интерфейса
- `StyleManager` — масштаб, стиль, реакция на resize
- `FontManager` — загрузка и обновление шрифтов
- `FileDialogManager` — файловые диалоги

## Код

- [App/GUI/interface/interface.h](../../../App/GUI/interface/interface.h)
- [App/GUI/interface/interface.cpp](../../../App/GUI/interface/interface.cpp)
- [App/GUI/interface/UiState.h](../../../App/GUI/interface/UiState.h)
- [App/GUI/interface/style/StyleManager.h](../../../App/GUI/interface/style/StyleManager.h)
- [App/GUI/interface/font_manager/FontManager.h](../../../App/GUI/interface/font_manager/FontManager.h)
- [App/GUI/interface/file_dialog/FileDialogManager.h](../../../App/GUI/interface/file_dialog/FileDialogManager.h)
- [App/GUI/interface/panels/settings/SettingsPanel.cpp](../../../App/GUI/interface/panels/settings/SettingsPanel.cpp)
- [App/GUI/io/manager/EventManager.cpp](../../../App/GUI/io/manager/EventManager.cpp)
- [App/GUI/io/window_events/WindowEvents.cpp](../../../App/GUI/io/window_events/WindowEvents.cpp)

## `Interface`

`Interface` из `App/GUI/interface/interface.h` — это главный управляющий слой GUI.

Он:

- создает и завершает `ImGui` context
- инициализирует бэкенды `ImGui_ImplGlfw` и `ImGui_ImplWGPU`
- обновляет кадр UI через `update()`
- рисует `ImGui` draw data в текущий render pass через `draw(...)`

Также он агрегирует основные панели:

- `ToolsPanel`
- `IOPanel`
- `SideToolsPanel`
- `SimControlPanel`
- `PeriodicPanel`
- `StatsPanel`
- `SettingsPanel`
- `DebugPanel`

## `UiState`

`UiState` — это общий state контейнер UI-уровня.

Через него живут:

- pause/play состояние
- скорость симуляции
- выбранный атом
- tooltip и preview flags
- временные UI-флаги, влияющие на панели и инструменты

Это не физическое состояние мира, а прикладное состояние интерфейса.

## Панельный Слой

Панели являются точками взаимодействия пользователя с приложением.

Они:

- читают `UiState`
- читают данные из `Simulation`
- меняют настройки рендера и симуляции
- вызывают файловые и служебные действия

Особенно важна `SettingsPanel`, потому что она связывает GUI сразу с двумя границами:

- параметрами `Simulation`
- параметрами `RenderData`

## Слой Ввода

`App/GUI/io/` отвечает за слой ввода:

- `keyboard/`
- `mouse/`
- `window_events/`
- `manager/`

`EventManager` собирает обработку событий окна и инпута, а GUI уже использует результат этого слоя вместе с `ImGui`.

## Поток Кадра

В типичном кадре GUI участвует так:

```text
Application
  -> SceneViewport::renderFrame(...)
  -> Interface::update()
  -> `renderer.camera.update(...)`
  -> `renderer.drawShot(...)`
  -> `Interface::draw(renderer)`
```

Это важно: UI не рисуется отдельным графическим движком поверх всего независимо, а встраивается в текущий render pass приложения.

## Причины

Такой разрез полезен потому что:

- GUI изолирован от физического ядра
- UI-код не размазывается по `Application`
- `Interface` служит одной точкой входа для всех панелей
- стиль, шрифты и диалоги вынесены в отдельные подсистемы, а не смешаны в одном файле

## Состояние Модуля

С инженерной точки зрения `GUI` сейчас остается одной из самых плотных и наименее чисто разделенных частей проекта.

Здесь уже сходятся сразу несколько осей ответственности:

- lifecycle `ImGui`
- панели и `UiState`
- ввод и window events
- связь с `Simulation`
- связь с `RenderData` и viewport
- служебные сценарии вроде preview и file dialog

Это рабочая и полезная часть приложения, но именно она остается одним из главных кандидатов на дальнейшую декомпозицию и упрощение связей между подсистемами.

## Связи

- [Входная Точка](../README.md)
- [Приложение](./App.md)
- [Поток Данных](./DataFlow.md)
- [Локализация](./Localization.md)
- [Рендер](./Rendering.md)
- [Термины](./Glossary.md)
