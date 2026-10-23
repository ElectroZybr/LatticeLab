# Композиция C++ пакета

Пакет экспортирует одну точку входа в `compose.cpp`. Остальной код пакета — обычные
C++ классы, которые не знают о Lattice. Подробный контракт ядра (Context, Focus,
пересборка, снимки) — в `../Lattice/Docs/ru/Kernel/Composition.md`.

```cpp
#include <Lattice/Composition.hpp>
#include <vendor/Storage.hpp>
#include <vendor/Algorithm.hpp>
#include <vendor/Loop.hpp>

extern "C" void compose(NodeBuild root) {
    auto storage = root.emplace<vendor::Storage>("storage", 1024);
    auto algorithm = root.emplace<vendor::Algorithm, vendor::AlgorithmAPI>("algorithm", storage);
    algorithm.param("dt", &vendor::Algorithm::dt);

    auto loop = root.emplace<vendor::Loop>("loop", algorithm);
    loop.service(&vendor::Loop::run, &vendor::Loop::requestStop);
}
```

* Метаданные пакета — `extern "C" constinit const Lattice::Package lattice_package{...}`
  в том же файле; blueprint называется по его `id`; стартовая запись —
  `<id>.<instance> = "on"` в `lattice.toml`.
* `emplace` регистрирует тип и явно указанные API на месте; отдельных списков
  регистрации нет.
* Handles в аргументах `emplace` становятся зависимостями и превращаются в обычные
  `T&`, `T*`, `std::vector<T*>`. Ручной `dependsOn` не нужен.
* Экспорты принадлежат объекту: `obj.param`, `obj.action`, `obj.globalAction`.
* Долгий цикл объявляется через `obj.service(run, stop)`, без наследования `ServiceAPI`.
* Таблица (`Lattice::Table`) получает `view`/`addRow` через `Lattice::exportTable(obj)`.
* Ошибка в `compose` откатывает создаваемую ветку.

Код без собственной ветки и без зависимости от runtime (контракты, данные, алгоритмы)
оформляется библиотекой в `Libs/<Name>` без `compose.cpp`; пакеты
размещают её объекты сами.

Рабочие примеры: `Packages/ClassicMD/compose.cpp` (ветка симуляции и сервис),
`Packages/CLI/compose.cpp` (инфраструктура: views runtime, глобальные команды),
`Packages/StdIo/compose.cpp` (общий ресурс и коллекция API).

После изменения заголовков ядра бинарник `LatticeLab` нужно пересобрать
(`xmake build -r LatticeLab`): `Runtime` компилируется из заголовков в `main.cpp`,
и xmake не отслеживает изменения заголовков пакета.
