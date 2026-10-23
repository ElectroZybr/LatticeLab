# Graphics: ветка рендера окна

Пакет. `compose.cpp` собирает ветку рендера одного окна; классы — обычные (`Graphics.core`).
Ветку вкладывает Shell: `root.compose("Graphics", "render")`.

```text
Graphics:render                     объект Render: FrameAPI, владеет surface и курсором окна (param cursor)
├── Slot<WindowAPI>:window          choice Shell::Glfw
│   ├── GlfwKeyboard:keyboard
│   ├── GlfwMouse:mouse
│   └── GlfwWindow:glfw
├── Surface:surface                 → device (mount), window
├── TestViewport:viewport           узел вьюпорта: → device, camera
│   ├── Camera:camera
│   └── Slot<TransformController>:controller   choice Graphics::FreeCamera → camera, owner<Viewport>
```

Каждый вьюпорт — отдельный узел: `branch(name)` с камерой, затем `construct<TestViewport,
Viewport>` на нём же и слот контроллера, который находит свой вьюпорт через `owner<Viewport>()`.
`Render` рисует все вьюпорты ветки (`collect<Viewport>`), их может быть сколько угодно.

Курсор один — параметр `cursor` объекта `Render` (пиксели surface). Каждый кадр `Render`
находит верхний по `layer()` вьюпорт под курсором и передаёт ему локальную координату
(`hover`); остальные получают `hovered() == false`. GUI-панель — вьюпорт с большим слоем.

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
