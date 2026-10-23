# CLI: интерактивная оболочка runtime

Инфраструктурный пакет. `CLI` и терминалы — обычные классы; команды, параметры
и сервис объявляет `compose.cpp`.

```text
CLI
├── LocalTerminal:local        Terminal; params prompt, prompt_color, path_color
└── CLI:cli                    → терминалы; params theme.*; сервис ветки
    └── ServiceRunner:service
```

Глобальные команды (`ls`, `help`, `cd`, `tree`, `logo`) принадлежат объекту `CLI:cli`
и снимаются при его удалении. `CLI` получает `ExportsView`, `TreeView` и терминалы
через конструктор; `run(idle)` обслуживает терминалы, пока они открыты.

```toml
[Startup]
CLI = ["on"]
```

```sh
xmake build CLI.tests && xmake run CLI.tests
```
