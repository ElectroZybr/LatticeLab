# Actions: маршрутизация ввода

Модуль без своей ветки (`Actions.core`, без `compose.cpp`). `ActionRouter` связывает
триггеры устройств ввода с runtime-экспортами (действия и параметры). Роутер имеет
смысл только в контексте окна, поэтому его создаёт пакет Shell в своей ветке:

```text
Shell
└── ActionRouter:router
```

Shell передаёт роутер кейбиндам и циклу окна, а цикл вызывает `tick()` (glfw опрашивается
в главном потоке):

```cpp
auto router = root.emplace<ActionRouter>("router", root.exports(), root.id());
router->registerInput(keyboard);
router->bind("quit", "Ctrl+Q");
router->tick();                    // раз в кадр
router->unregisterInput(keyboard); // до уничтожения устройства
```

* `ActionRouter` — обычный класс: `ExportsView` и узел контекста приходят через конструктор.
* `InputAPI` — обычный интерфейс устройства ввода с виртуальным деструктором.
* Более специфичная цепочка модификаторов (`A+Shift`) подавляет общую (`A`).

```sh
xmake build Actions.tests && xmake run Actions.tests
```
