# Шаг симуляции и форма сосуда

Шаг ClassicMD — интегратор-слот и конвейер `step` из узлов; сосуд — узел `space` с формой
в виде строки (`rounded(sphere(25) + cylinder(5, 100), 1)`) и законами стенок. Проект
границ — [Boundaries.md](../articles/Boundaries.md); запись формы с ошибкой — [проверка
параметров](ParamValidation.md).

Дерево симуляции:

```text
ClassicMD::Universe1 [service]           шаг: integrator.step()
├── commands, data, atoms
├── space            Domain              shape (строка, свой сеттер), size, periodic
│   └── laws         BoundaryLaws        restitution [0…1], soft: stiffness, range
├── integrator       slot IntegratorAPI  Verlet: dt — predict → step.run() → correct
└── step             Pipeline            всё между половинами интегратора, по порядку детей
    ├── boundary     Pipeline            заворот, выталкивание из стенок
    └── forces       Pipeline            мягкие стенки; позже LJ, coulomb
```

## Язык форм (`libs/boundaries`, модель)

- [ ] `parseShape(text)` → `Shape` или ошибка с позицией (код, сообщение, участок, ожидаемые имена)
- [ ] Примитивы `cube`, `box`/`rectangle`, `sphere`, `cylinder`, `capsule`, `halfspace`; операторы `+ - &`; `smooth`, `rounded`, `shell`, `move`, `rotate` (градусы), `scale`
- [ ] Тесты: каждая конструкция совпадает с формой из `Shape` на сетке точек; ошибки — с верной позицией

## ClassicMD

- [ ] `ParticleDynamics::IntegratorAPI { step() }`; `Verlet(atoms, step)`: `predict` → `step.run()` → `correct`
- [ ] Интегратор — слот (`ClassicMD::Verlet`); симуляция получает его из `configure` (`setIntegrator`), смена слота её не задевает
- [ ] `Domain`: коробка и `Cpu::Geometry` внутри; `shape` со своим сеттером (`parseShape`), `size`, `periodic`; смена поднимает версию границ
- [ ] `BoundaryLaws` (ребёнок `space`): `Cpu::Walls` и материалы внутри; `constrain`/`addForces` — стадии `boundary` и `forces`; параметры законов с диапазонами
- [ ] `step` — `Pipeline` с группами `boundary` и `forces`; переходник `Boundary` и узлы `geometry`/`walls` удалены
- [ ] Тесты: порядок фаз, смена формы сеттером, частицы внутри и энергия, отказ на неверной формуле

## Потом

- [ ] Смена варианта слота из CLI (retire → maintain → choice)
- [ ] Коробка из габаритов формы
