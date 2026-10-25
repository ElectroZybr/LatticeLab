# Integrators

Статическая библиотека (`libs/`, цель `Integrators`): интеграторы как половины шага (`predict`, `correct`), которые композиция ставит стадиями конвейера вокруг сил.
Подключение: `#include <integrators/Verlet.hpp>`, в xmake — `add_deps("Integrators")`.
Пакет размещает интегратор в своей ветке, например ClassicMD:

```cpp
auto verlet = root.emplace<Integrators::Verlet>("integrator", atoms);
step.emplace<Lattice::CallStage<Integrators::Verlet>, Lattice::PipelineAPI>("predict", verlet, &Integrators::Verlet::predict);
verlet.param("dt", &Integrators::Verlet::dt);
```

`Verlet(ParticleStorage&)` добавляет свои колонки при создании и удаляет при уничтожении.
`KDK` — заглушка; `integrators/KDK.cpp` и `integrators/StepOps.hpp` ещё не перенесены.

```sh
xmake build Integrators.tests && xmake run Integrators.tests
```
