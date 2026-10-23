# Scene: сцена окна

Пакет. `compose.cpp` собирает сцену одного окна кодом из классов
[Graphics](../Graphics/README.md); ветку вкладывает Shell: `root.compose("Scene", "render")`.

```text
Scene:render                        объект Render: FrameAPI, курсор (param cursor), depth
├── Slot<WindowAPI>:window          choice Shell::Glfw: GlfwWindow на самом слоте
│   ├── GlfwKeyboard:keyboard
│   └── GlfwMouse:mouse
├── Surface:surface                 → device (mount), window
├── RenderLayout:layout             форматы, layout группы камеры
├── TestObject:marker               объект сцены (SceneObject); param position
└── main                            вьюпорт: объект Viewport на узле ветки → layout, camera
    ├── Camera:camera               param position
    └── Slot<TransformController>:controller   choice Scene::FreeCamera → camera, owner<Viewport>
```

* **Объекты сцены** — прямые дети ветки с API `SceneObject`: их рисует каждый вьюпорт.
  Свои объекты вьюпорта (`SceneObject`-дети его узла) рисуются только в нём.
* **`configure`** назначает наборы по текущему дереву: вьюпорты `Render`, объекты и
  контроллер каждого вьюпорта. Добавленный узел подхватывается в `maintain`.
* **Цель ввода** — `Focus<InputTargetAPI>` ветки: `Render` сообщает о вьюпорте под
  курсором (`onHover`), композиция выбирает его контроллер. Роутеру цель назначает Shell.
* Сохранение состояния (камеры, параметры объектов) — экспорты `param` и `Serializer`.
