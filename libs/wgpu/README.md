# Wgpu: GPU-вычисления

Библиотека над wgpu-native (namespace `Wgpu`), без runtime Lattice и без рендера.

| Класс | Назначение |
|---|---|
| `Instance`, `Device` | Устройство, очередь, `writeBuffer`; `ResourceRegistry` — учёт живых ресурсов; `errorCount()` — ошибки валидации |
| `Buffer`, `Texture`, `Shader` | RAII-ресурсы (учитываются в реестре устройства) |
| `BindingLayout`, `BindingSet` | Явные layout'ы bind group и наборы привязок (общие для разных pipeline) |
| `CommandList` | Пакет команд: `copyBuffer`, `submit`; кодировщик для проходов (render — в Graphics) |
| `ComputePipeline`, `ComputePass` | Compute-шейдер на явных layout'ах; проход (RAII): `setPipeline`, `setBindings`, `dispatch`; `workgroups(n, size)` |
| `Device::readBuffer`, `Device::wait` | Чтение буфера на CPU с ожиданием (тесты, проверки — не путь кадра), ожидание завершения работы GPU |
| `GpuSoA` | Таблица колонок на GPU (`Lattice::Table`); зеркало CPU-таблицы (`CpuSource`: копия — команда очереди владельца) или GPU-владеемая |

Рендерные примитивы (`Surface`, `RenderPass`, `RenderPipeline`) — в `Graphics.core`: они
строятся на этой библиотеке, но вычислительные бэкенды их не видят. Пакет `WGPU` публикует
общий `Device`.

```sh
xmake build Wgpu.tests && xmake run Wgpu.tests
xmake build Wgpu.bench && xmake run Wgpu.bench [имя]   # GPU против CPU, загрузка, чтение
```
