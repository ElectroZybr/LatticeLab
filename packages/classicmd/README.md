# ClassicMD

Пакет Lattice: `compose.cpp` собирает ветку классической MD. Остальное — обычные классы.

```text
ClassicMD:<name> [service]     ClassicMD (shared ParticleSystemAPI) → atoms, step, commands, boundary
│                              param cell_size, action step
├── Queue:commands             очередь потока симуляции
├── AtomData:data              NamedSoA, Table (view/addRow)
├── AtomStorage:atoms          ParticleStorage, Table (view/addRow)
├── Space:space                коробка и периодичность
├── Geometry:geometry          SDF-программа сосуда (Boundaries::Cpu)
├── Walls:walls                законы стенок по материалам
├── Boundary:boundary          заворот и стенки над atoms; показывает границы читателям
├── Verlet:integrator          param dt
└── step                       Pipeline: шаг симуляции, стадии по порядку
    ├── predict                CallStage<Verlet>: позиции, prev ← F, F = 0
    ├── constraints            Pipeline
    │   └── walls              CallStage<Boundary>::constrain
    ├── forces                 Pipeline
    │   └── walls              CallStage<Boundary>::addForces
    └── correct                CallStage<Verlet>: скорости
```

Шаг — конвейер узлов (`Lattice::Pipeline`, ядро): новая сила — узел в `forces`, порядок —
порядок детей. `configure` пакета назначает стадии (`Lattice::configurePipelines`).

`ClassicMD::run()` шагает симуляцию с периодом (по умолчанию 16 мс) до `requestStop` и
между шагами выполняет команды очереди `commands` (`Tasks::Queue`). Поток `run` —
единственный владелец состояния. Экспорты привязаны к очереди (`executeOn(commands)`).
Копия частиц и границ для рендера — тоже команды. Без `run` команды выполняются сразу
у вызывающего.

Симуляция не знает о рендере и WGPU. Для читателей она публикует себя как
`ParticleSystemAPI`: частицы и `boundaries()` (коробка, программа, версия).

```toml
[Startup]
ClassicMD.Universe1 = "on"
```

## Использование вне runtime

```cpp
ClassicMD::AtomStorage atoms;
Integrators::Verlet verlet{atoms};
Lattice::CallStage<Integrators::Verlet> predict{verlet, &Integrators::Verlet::predict};
Lattice::CallStage<Integrators::Verlet> correct{verlet, &Integrators::Verlet::correct};
Lattice::Pipeline step;
step.setStages({&predict, &correct});
Tasks::Queue commands;
ClassicMD::ClassicMD simulation{atoms, step, commands};
simulation.step();
```

```sh
xmake build ClassicMD.tests && xmake run ClassicMD.tests
```
