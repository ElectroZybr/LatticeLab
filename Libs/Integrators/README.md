# Integrators

Статическая библиотека (`Libs/`, цель `Integrators`): реализации `ParticleDynamics::IntegratorAPI`.
Подключение: `#include <Integrators/Verlet.hpp>`, в xmake — `add_deps("Integrators")`.
Пакет размещает интегратор в своей ветке, например ClassicMD:

```cpp
auto verlet = root.emplace<Integrators::Verlet, ParticleDynamics::IntegratorAPI>("integrator", atoms);
verlet.param("dt", &Integrators::Verlet::dt);
```

`Verlet(ParticleStorage&)` добавляет свои колонки при создании и удаляет при уничтожении.
`KDK` — заглушка; `Integrators/KDK.cpp` и `Integrators/StepOps.hpp` ещё не перенесены.

```sh
xmake build Integrators.tests && xmake run Integrators.tests
```
