> Язык: RU
> Статус EN: создан каркас
# Benchmarks

Эта статья описывает benchmark-инфраструктуру проекта: что именно измеряется, как это собирается и как этим пользоваться на практике.

Benchmark-слой нужен для двух задач:

- измерять производительность отдельных частей движка и рендера
- давать изолированные сценарии для локального анализа без полного UI-потока

То есть benchmark'и в проекте используются не только как "таблица чисел", но и как технический способ гонять части системы отдельно.

## Структура

В проекте есть два основных benchmark-направления:

- `Lattice/benchmarks/` — benchmark'и движка
- `Rendering/benchmarks/` — benchmark'и рендера

На уровне сборки они объединяются в один benchmark target.

## Код

- [Lattice/benchmarks/CMakeLists.txt](../../../Lattice/benchmarks/CMakeLists.txt)
- [Lattice/benchmarks/bench_main.cpp](../../../Lattice/benchmarks/bench_main.cpp)
- [Lattice/benchmarks/Fixture.h](../../../Lattice/benchmarks/Fixture.h)
- [Lattice/benchmarks/SceneBuilders.h](../../../Lattice/benchmarks/SceneBuilders.h)
- [Rendering/benchmarks/CMakeLists.txt](../../../Rendering/benchmarks/CMakeLists.txt)
- [Rendering/benchmarks/Fixture.h](../../../Rendering/benchmarks/Fixture.h)
- [Rendering/benchmarks/SceneBuilders.h](../../../Rendering/benchmarks/SceneBuilders.h)
- [Rendering/benchmarks/BM_RenderAtoms.cpp](../../../Rendering/benchmarks/BM_RenderAtoms.cpp)

## Сборка

Для benchmark'ов используется preset `bench`.

Конфигурация:

```bash
cmake --preset bench
```

Сборка:

```bash
cmake --build --preset bench
```

Этот preset включает:

- `Release`
- `BUILD_BENCHMARKS=ON`

## Бинарники

После сборки обычно используются два исполняемых файла:

- `build/bench/benchmarks` — основной binary на базе Google Benchmark
- `Lattice/benchmarks/bench` — runner-обертка для запуска и оформления результатов

## Основной Benchmark Binary

`benchmarks` — это прямой benchmark binary.

Он собирает:

- benchmark'и движка из `Lattice/benchmarks/physics/`
- benchmark'и рендера из `Rendering/benchmarks/`
- общую benchmark-инфраструктуру

Дополнительные пользовательские аргументы, поддерживаемые через `bench_main.cpp`:

- `--scene`
- `--degradation`
- `--warmup-steps`

Остальные аргументы передаются стандартному Google Benchmark.

Примеры:

```bash
./build/bench/benchmarks
./build/bench/benchmarks --benchmark_filter=Render
./build/bench/benchmarks --scene default
./build/bench/benchmarks --scene default --benchmark_filter=RenderGrid
```

## Runner

`Lattice/benchmarks/bench` — это отдельный runner для более удобного запуска benchmark'ов.

Он полезен, когда нужно:

- запускать benchmark'и в более управляемом режиме
- сохранять результаты
- сравнивать прогоны
- получать более удобный вывод, чем сырой stdout Google Benchmark

BmRunner`отвечает за:

- конфигурацию запуска
- оформление вывода
- сохранение json-результатов
- поддержку baseline/last run сценариев

## Benchmark'и Движка

`Lattice/benchmarks/physics/` содержит benchmark'и по ключевым стадиям симуляции.

Типичные примеры:

- `BM_SimulationStep.cpp`
- `BM_ComputeForces.cpp`
- `BM_NeighborListRebuild.cpp`
- `BM_SpatialGridRebuild.cpp`
- `BM_AtomsSort.cpp`
- `BM_PostProcessVelocities.cpp`

Они нужны, чтобы отдельно видеть стоимость:

- вычисления сил
- перестройки neighbor search
- отдельных стадий интегратора
- полного шага симуляции

## Benchmark'и Рендера

`Rendering/benchmarks/` содержит benchmark'и по отдельным частям визуализации.

Основные файлы:

- `BM_RenderAtoms.cpp`
- `BM_RenderGrid.cpp`
- `BM_RenderField.cpp`
- `BM_RenderBHtree.cpp`

Типичный паттерн здесь такой:

- подготовка CPU-данных
- upload на GPU
- draw отдельного типа визуализации

За счет этого можно измерять не только "рендер целиком", но и отдельные стадии.

## Как Добавить

Новый benchmark обычно добавляется по одному из двух путей:

- benchmark движка — в `Lattice/benchmarks/physics/`
- benchmark рендера — в `Rendering/benchmarks/`

Обычный порядок такой:

1. Создать новый файл `BM_*.cpp` в `Lattice/benchmarks/physics/`.
2. Подключить `benchmark/benchmark.h` и нужные заголовки движка.
3. Использовать существующие `Fixture`, `SceneBuilders` или локальную подготовку сцены.
4. Зарегистрировать benchmark через стандартные макросы Google Benchmark.
5. Пересобрать preset `bench`.

Важно: отдельной правки `CMakeLists.txt` обычно не требуется, потому что benchmark'и в этой директории подхватываются через `GLOB`.

### Практический Принцип

Хороший benchmark в этом проекте обычно:

- измеряет одну понятную стадию
- использует воспроизводимую сцену
- не смешивает несколько источников стоимости без необходимости
- может быть сопоставлен с соседними benchmark'ами

## Fixture

`Fixture` в benchmark'ах нужен для того, чтобы не собирать одинаковую инфраструктуру заново в каждом `BM_*.cpp`.

Его задача:

- поднять общую benchmark-среду
- подготовить повторяемый setup
- дать benchmark-кейсу удобный доступ к уже готовым объектам

В проекте сейчас есть два основных fixture-типа:

- `Lattice/benchmarks/Fixture.h`
- `Rendering/benchmarks/Fixture.h`

### Fixture Движка

Fixture движка строится вокруг `Lattice::Simulation`.

Он уже берет на себя:

- создание `Simulation`
- создание мира
- выбор параметров сцены из benchmark-аргументов
- rebuild сцены
- прогрев сцены
- подготовку neighbor list
- helper'ы для шагов интеграции

Поэтому benchmark физики обычно не должен каждый раз вручную:

- создавать `Simulation`
- создавать `World`
- заполнять стартовую сцену
- повторять один и тот же setup-код

### Fixture Рендера

Fixture рендера строится вокруг `Renderer3D` без окна и отдельной целевой текстуры кадра.

Он уже берет на себя:

- `WGPUContext` без окна
- создание рендерера
- настройку screen size
- создание целевой текстуры кадра
- хранение benchmark-сцены
- `syncRenderer()`
- `renderFrame()`
- базовые counters

Это значит, что rendering benchmark может сразу концентрироваться на измеряемой стадии:

- prepare
- upload
- draw
- full frame

### Fixture И Scene Builders

Fixture и scene builders выполняют разные роли:

- fixture отвечает за benchmark-среду
- scene builder отвечает за содержимое сцены

На практике это разделение выглядит так:

- в движке используется `SceneBuilders`
- в рендере используются `SceneBuilders`

Это удобно, потому что позволяет переиспользовать один и тот же setup, меняя только входную сцену.

### Когда Использовать Fixture

Fixture стоит использовать почти всегда, если benchmark:

- зависит от `Simulation` или `Renderer`
- требует повторяемой сцены
- требует общего setup/teardown
- должен быть сопоставим с соседними benchmark'ами

Отказываться от fixture имеет смысл только для совсем маленького benchmark'а, которому не нужна общая инфраструктура.

## Результаты

В benchmark-инфраструктуре движка уже есть директория:

- `Lattice/benchmarks/results/`

Там встречаются, например:

- `baseline.json`
- `last_run.json`

Это полезно для:

- сравнения текущего запуска с эталоном
- отслеживания деградаций
- фиксации прогонов перед крупными изменениями

## Практика Использования

Хороший рабочий порядок обычно такой:

1. Собрать preset `bench`.
2. Запустить конкретный benchmark или группу через `--benchmark_filter`.
3. Повторить замер на одной и той же сцене.
4. Сравнить цифры до и после изменения.
5. Если нужен более удобный workflow — использовать runner.

## Ссылки

- [Рендер](./Rendering.md)
- [Движок Lattice](./Lattice.md)
- [Входная Точка](Docs/ru/Main.md)
