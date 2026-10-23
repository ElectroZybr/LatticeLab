# Graphics: классы рендера

Модуль без своей ветки: `Graphics.core` — обычные классы над WGPU. Сцену окна из них
собирает пакет [Scene](../Scene/README.md).

| Класс | Назначение |
|---|---|
| `Viewport` | Вид сцены: область surface (`ViewportArea`), камера, объекты. Владеет uniform камеры — bind group 0 всех pipeline объектов, поэтому один объект рисуется в нескольких вьюпортах с разными камерами |
| `ViewportArea` | Геометрия области и курсор над ней, без GPU (её читают контроллеры) |
| `SceneObject` | Всё, что рисуется: transform и `render(pass, ViewState)`; `ViewState` — прямоугольник, `CameraUniform`, группа камеры |
| `RenderLayout` | Общее для pipeline окна: форматы цвета и глубины, layout группы камеры |
| `Render` | Кадр окна: `update()` контроллеров вьюпортов, `render()` вьюпортов по слоям, depth-цель, present; единственный курсор окна |
| `TestObject` | Простейший объект: диск в своём transform (`shaders/circle.wgsl`), свои layout/uniform группы 1 |
| `Camera`, `Ray` | Камера и математика лучей |
| `FreeCameraController`, `OrbitCameraController`, … | `TransformController`: ввод копится в публичных полях, применяется в `update()`; цель ввода — `InputTargetAPI` |

Курсор один — параметр `cursor` объекта `Render` (пиксели surface). Каждый кадр `Render`
находит верхний по `layer()` вьюпорт под курсором и передаёт ему локальную координату
(`hover`); остальные получают `hovered() == false`. GUI-панель — вьюпорт с большим слоем.

Pipeline объекта строится на `RenderLayout`: `layouts = {&layout.camera(), <свои группы>}`,
`colorFormat`/`depthFormat` из него же. Камера в WGSL:

```wgsl
struct Camera { viewProjection: mat4x4<f32>, view: mat4x4<f32>, projection: mat4x4<f32>, position: vec4<f32> };
@group(0) @binding(0) var<uniform> camera: Camera;
```

GPU-ресурсы — RAII-поля компонентов и строки в реестре устройства (`resources`).

```sh
xmake build Graphics.tests && xmake run Graphics.tests
```
