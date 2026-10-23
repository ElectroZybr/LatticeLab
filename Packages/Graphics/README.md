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
├── viewport
│   ├── Camera:camera
│   ├── TestViewport:view           → device, camera; param cursor
│   └── Slot<TransformController>:controller   choice Graphics::FreeCamera (look, move, zoom, pan)
└── Render:render                   FrameAPI → device, window, surface, viewports, controllers
```

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
