# ParticleDynamics: контракты и хранилище частиц

Header-only библиотека (`Libs/`, цель `ParticleDynamics`): обычные контракты и данные без runtime.
Подключение: `#include <ParticleDynamics/ParticleStorage.hpp>`, в xmake — `add_deps("ParticleDynamics")`.

| Файл | Содержимое |
|---|---|
| `ParticleAPI.hpp` | `IntegratorAPI`, `ForceFieldAPI`, `SpatialIndexAPI` — интерфейсы с виртуальным деструктором |
| `ParticleStorage.hpp` | `ParticleStorage : StdData::SoA` — позиции, скорости, силы; подвижные частицы первыми |
| `SpatialGrid.hpp` | `SpatialGrid(ParticleStorage&)` — сетка ячеек с ghost-слоями |

`ParticleDynamics/NeighborList.*` ещё не перенесён и не собирается.

```sh
xmake build ParticleDynamics.tests && xmake run ParticleDynamics.tests
```
