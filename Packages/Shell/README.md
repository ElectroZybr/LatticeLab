# Shell: окно приложения

Пакет приложения: кейбинды, вложенная ветка рендера (Graphics) и цикл окна как host-сервис. Окно, ввод и цикл — обычные классы (`Shell.core`).

```text
Shell
├── KeybindsLoader:keybinds     → router (mount), io (mount); action load
├── Graphics:render             ветка пакета Graphics (окно, ввод, surface, сцена, Render)
└── WindowSystem:loop           → router, windows, inputs, frames (collect); host-сервис
```

`WindowSystem` регистрирует устройства ввода в общем `ActionRouter` и снимает их в
деструкторе. Цикл вызывает `idle` (обслуживание runtime), опрашивает окно, вызывает
`router.tick()` и `frame()` у всех `FrameAPI` (Render). Окно закрыто или `quit` — цикл выходит.

```toml
[Startup]
StdIo = "on"
Actions = "on"
WGPU = "on"
Shell = ["on", "host"]
```

```sh
xmake build Shell.tests && xmake run Shell.tests
```
