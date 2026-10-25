# StdData: табличные структуры данных

Header-only библиотека (`libs/`, цель `StdData`): не пакет Lattice и не зависит от runtime.
Подключение: `#include <stddata/SoA.hpp>`, в xmake — `add_deps("StdData")`.

| Класс | Назначение |
|---|---|
| `SoA` | Structure of Arrays с колонками-тегами; реализует `Lattice::Table` |
| `NamedSoA` | `SoA` + поиск строк по имени |
| `CSR` | Сжатые строки переменной длины |

`Lattice::Table` — header-only контракт. Действия `view`/`addRow` объекту даёт
композиция: `Lattice::exportTable(obj)`.

```sh
xmake build StdData.tests && xmake run StdData.tests
```
