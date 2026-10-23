# CLI: интерактивная оболочка runtime

Инфраструктурный пакет. `CLI` и терминалы — обычные классы; команды, параметры
и сервис объявляет `compose.cpp`.

```text
CLI [service]                 params theme.*
└── LocalTerminal:local        Terminal; params prompt, prompt_color, path_color
```

Глобальные команды (`ls`, `help`, `cd`, `tree`, `logo`) принадлежат корневому объекту `CLI`
и снимаются при его удалении. `CLI` получает `ExportsView`, `TreeView` и терминалы
через конструктор; `run(idle)` обслуживает терминалы, пока они открыты.

```toml
[Startup]
CLI = ["on"]
```

```sh
xmake build CLI.tests && xmake run CLI.tests
```

Присваивание `<параметр> <значение>` — явное пользовательское переопределение,
передаваемое через `ExportsView::set`. `reset <параметр>` или `reset #<exportId>`
удаляет переопределение из слоя Serialization, сохраняя текущее значение поля.
Исходное значение вернётся при новой сборке ветки без переопределения.
