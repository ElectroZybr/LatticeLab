# Shell: окно приложения

Пакет приложения: кейбинды, окно с устройствами ввода, вложенная ветка рендера (Graphics) и цикл окна как host-сервис. Окно, ввод и цикл — обычные классы (`Shell.core`).

```text
Shell [service]               WindowSystem
├── ActionRouter:router         роутер ввода (модуль Actions)
├── KeybindsLoader:keybinds     → router, io (mount); action load
├── Slot<WindowAPI>:window      choice Shell::Glfw: GlfwWindow на самом слоте
│   ├── GlfwKeyboard:keyboard
│   └── GlfwMouse:mouse
└── Graphics:render             ветка пакета Graphics: Render, GPU-данные, вьюпорты-сцены
```

Окна, устройства ввода и кадры `WindowSystem` получает в `configure` Shell (весь
поддерево, вместе с веткой рендера): новые устройства регистрируются в `ActionRouter`,
убранные снимаются. Там же роутеру назначается цель кейбиндов: текущий
`Focus<InputTargetAPI>` ветки кадра, а пока он не выбран — узел самого кадра (`Render`,
его локальный `cursor`). Несколько кадров на один роутер пока не поддерживаются — ошибка. Цикл вызывает `idle` (обслуживание runtime), опрашивает окно, вызывает
`router.tick()` и `frame()` у всех `FrameAPI` (Render). Окно закрыто или `quit` — цикл выходит.

```toml
[Startup]
StdIo = "on"
WGPU = "on"
Shell = ["on", "host"]
```

```sh
xmake build Shell.tests && xmake run Shell.tests
```
