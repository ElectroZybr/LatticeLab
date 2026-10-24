# ClassicMD

Пакет Lattice: `compose.cpp` собирает ветку классической MD. Остальное — обычные классы.

```text
ClassicMD:<name> [service]     ClassicMD (shared ParticleSystemAPI) → atoms, integrator, commands
│                              param cell_size, action step
├── Queue:commands             очередь потока симуляции
├── AtomData:data              NamedSoA, Table (view/addRow)
├── AtomStorage:atoms          ParticleStorage, Table (view/addRow)
└── Verlet:integrator          IntegratorAPI → atoms; param dt
```

`ClassicMD::run()` шагает симуляцию с периодом (по умолчанию 1 с) до `requestStop` и
между шагами выполняет команды очереди `commands` (`Tasks::Queue`). Поток `run` —
единственный владелец состояния. Экспорты всех объектов ветки привязаны к очереди
(`executeOn(commands)`): `step`, `dt`, `cell_size`, `view`/`addRow` таблиц выполняются
между шагами, запись возвращается после применения. Копия частиц для рендера — тоже
команда. Без `run` команды выполняются сразу у вызывающего.

Симуляция не знает о рендере и WGPU. Для читателей она публикует себя как
`ParticleSystemAPI`; GPU-копию делает сцена (пакет Scene).

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
