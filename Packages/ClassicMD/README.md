# ClassicMD

Пакет Lattice: `compose.cpp` собирает ветку классической MD. Остальное — обычные классы.

```text
ClassicMD:<name>
├── AtomData:data              NamedSoA, Table (view/addRow)
├── AtomStorage:atoms          ParticleStorage, Table (view/addRow)
├── Verlet:integrator          IntegratorAPI → atoms; param dt
├── ClassicMD:simulation       → atoms, integrator; param cell_size, action step
└── SimulationLoop:loop        → simulation; сервис ветки
    └── ServiceRunner:service
```

`SimulationLoop` шагает симуляцию с периодом (по умолчанию 1 с) до `requestStop`;
композиция объявляет его сервисом через `loop.service(...)`. Запуск:

```toml
[Startup]
ClassicMD.Universe1 = "on"
```

## Использование вне runtime

```cpp
ClassicMD::AtomStorage atoms;
Integrators::Verlet verlet{atoms};
ClassicMD::ClassicMD simulation{atoms, verlet};
simulation.step();
```

```sh
xmake build ClassicMD.tests && xmake run ClassicMD.tests
```
