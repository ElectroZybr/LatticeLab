# Shell: окно приложения

Пакет приложения: кейбинды, вложенная ветка рендера (Graphics) и цикл окна как host-сервис. Окно, ввод и цикл — обычные классы (`Shell.core`).

```text
Shell [service]               WindowSystem
├── ActionRouter:router         роутер ввода (модуль Actions)
├── KeybindsLoader:keybinds     → router, io (mount); action load
└── Graphics:render             ветка пакета Graphics (окно, ввод, surface, сцена, Render)
```

`WindowSystem` регистрирует устройства ввода в `ActionRouter` ветки и снимает их в
деструкторе. Цикл вызывает `idle` (обслуживание runtime), опрашивает окно, вызывает
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
