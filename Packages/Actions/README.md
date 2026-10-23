# Actions: маршрутизация ввода

Инфраструктурный пакет. `compose.cpp` публикует один общий `ActionRouter`, который
связывает триггеры устройств ввода с runtime-экспортами (действия и параметры).

```text
Actions
└── ActionRouter:router        shared; кандидат Context для mount
```

Владелец устройств ввода (Shell) получает роутер и сам вызывает `tick()` из своего
цикла ввода (glfw опрашивается в главном потоке):

```cpp
auto router = branch.mount<ActionRouter>().ref();
router->registerInput(keyboard);
router->bind("quit", "Ctrl+Q");
router->tick();                    // раз в кадр
router->unregisterInput(keyboard); // до уничтожения устройства
```

* `ActionRouter` — обычный класс: `ExportsView` и узел контекста приходят через конструктор.
* `InputAPI` — обычный интерфейс устройства ввода с виртуальным деструктором.
* Более специфичная цепочка модификаторов (`A+Shift`) подавляет общую (`A`).

```toml
[Startup]
Actions = "on"   # до веток, которые монтируют роутер
```

```sh
xmake build Actions.tests && xmake run Actions.tests
```
