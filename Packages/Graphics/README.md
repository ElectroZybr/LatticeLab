# Graphics: ветка рендера окна

Пакет. `compose.cpp` собирает ветку рендера одного окна из обычных классов
(`Graphics.core`); ветку вкладывает Shell: `root.compose("Graphics", "render")`. Окно и
устройства ввода принадлежат Shell, ветка берёт окно из окружения (`require<WindowAPI>`).

```text
Graphics:render                     объект Render: FrameAPI; surface, depth, RenderLayout внутри;
│                                   param cursor
├── GpuSoA:particles                GPU-копия частиц симуляции (mount ParticleSystemAPI)
└── main                            вьюпорт — сцена: объект Viewport на узле ветки
    ├── Camera:camera               param position
    ├── Slot<TransformController>:controller   choice Graphics::FreeCamera → camera, owner<Viewport>
    └── Spheres:particles           объект сцены → particles (колонки Pos::X/Y/Z)
```

* **Вьюпорт — это сцена**: область surface, камера, контроллер и объекты сцены в его
  детях. Вьюпортов может быть несколько, каждый со своими объектами; общие данные
  (`GpuSoA`) принадлежат `Render` и используются любым из них.
* **Внутреннее — не узлы.** `Surface`, depth и `RenderLayout` — члены `Render`. Вьюпорты и
  объекты получают `render->layout()` обычной ссылкой при создании и позже к нему не
  обращаются (объект ветки уничтожается раньше детей).
* **GPU-копия** (`GpuSoA` с `CpuSource`): когда рендер забирает копию, следующая
  запрашивается командой очереди симуляции — копирование согласовано с шагом. Удаление
  ветки освобождает копию (`detach` при retire, уничтожение после `idle()`), симуляция
  продолжает работу.
* **`configure`** назначает вьюпорты `Render`, объекты и контроллер каждого вьюпорта.
* **Цель ввода** — `Focus<InputTargetAPI>` ветки: `Render` сообщает о вьюпорте под курсором
  (`onHover`), композиция выбирает его контроллер; роутеру цель назначает Shell.

| Класс | Назначение |
|---|---|
| `Render` | Кадр окна: `update()` контроллеров, `render()` вьюпортов по слоям, present; surface, depth, `RenderLayout`, курсор |
| `Viewport` | Сцена: область (`ViewportArea`), камера, объекты; uniform камеры — bind group 0 всех pipeline объектов |
| `ViewportArea` | Геометрия области и курсор над ней, без GPU (её читают контроллеры) |
| `SceneObject` | Всё, что рисуется: transform и `render(pass, ViewState)` |
| `Spheres` | Сферы из GPU-таблицы по колонкам, палитра «вид → цвет, радиус», `frag_depth` |
| `RenderLayout` | Общее для pipeline окна: форматы цвета и глубины, layout группы камеры |
| `TestObject` | Простейший объект: диск в своём transform (тесты, `shaders/circle.wgsl`) |
| `Camera`, `Ray`, `FreeCameraController`, … | Камера, лучи, контроллеры (`TransformController`, цель ввода `InputTargetAPI`) |

```wgsl
struct Camera { viewProjection: mat4x4<f32>, view: mat4x4<f32>, projection: mat4x4<f32>, position: vec4<f32> };
@group(0) @binding(0) var<uniform> camera: Camera;
```

```sh
xmake build Graphics.tests && xmake run Graphics.tests
```
