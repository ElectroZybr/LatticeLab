# Graphics: ветка рендера окна

Пакет. `compose.cpp` собирает ветку рендера одного окна; классы — обычные (`Graphics.core`).
Ветку вкладывает Shell: `root.compose("Graphics", "render")`.

```text
Graphics:render
├── Slot<WindowAPI>:window          choice Shell::Glfw
│   ├── glfwKeyboard:keyboard
│   ├── glfwMouse:mouse
│   └── glfwWindow:glfw
├── Surface:surface                 → device (mount), window
├── TestViewport:viewport           узел вьюпорта: → device, camera; param cursor
│   ├── Camera:camera
│   └── Slot<TransformController>:controller   choice Graphics::FreeCamera → camera, owner<Viewport>
└── Render:render                   FrameAPI → device, window, surface, viewports, controllers
```

Каждый вьюпорт — отдельный узел: `branch(name)` с камерой, затем `construct<TestViewport,
Viewport>` на нём же и слот контроллера, который находит свой вьюпорт через `owner<Viewport>()`.
`Render` рисует все вьюпорты ветки (`collect<Viewport>`), их может быть сколько угодно.

| Класс | Назначение |
|---|---|
| `Render` | Кадр: `update()` контроллеров, `render()` viewport'ов, present |
| `Viewport`, `TestViewport` | Области поверхности; `TestViewport` владеет uniform, shader, pipeline, bindings |
| `Camera`, `SceneObject`, `Ray` | Камера и математика лучей |
| `FreeCameraController`, `OrbitCameraController`, … | Ввод копится в публичных полях и применяется в `update()` |

GPU-ресурсы — RAII-поля компонентов и строки в реестре устройства (`resources`).
Шейдеры — в `shaders/`.

```sh
xmake build Graphics.tests && xmake run Graphics.tests
```

Управление камерой (`Config/keybinds.toml`): WASD — движение, Q/E — вниз/вверх,
правая кнопка + движение мыши — поворот, средняя — панорамирование, колесо — приближение.
