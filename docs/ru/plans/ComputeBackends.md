# Бэкенды вычислений

Модуль алгоритма — модель + бэкенды рядом: `libs/<name>/` с целями `<Name>` (модель),
`<Name>.cpu`, `<Name>.wgpu`, позже `<Name>.cuda`. Нижние библиотеки — по бэкенду:
`libs/wgpu` для GPU, `libs/tasks` для потоков CPU.

## libs/wgpu

- [x] Вычислительная GPU-библиотека (namespace `Wgpu`): Device, Buffer, Texture, Shader, BindingLayout/BindingSet, CommandList, ошибки, учёт ресурсов, `GpuSoA`; без рендера; `packages/wgpu` — только `compose.cpp`
- [x] Рендерные части (`Surface`, `RenderPass`, `RenderPipeline`) — в `Graphics.core`; `NativeWindow` — в Shell, Shell не зависит от GPU
- [x] `ComputePipeline`, `ComputePass`, `Device::readBuffer`/`wait`; тесты со сверкой данных; бенчи GPU против CPU, загрузки и чтения
- [x] `Wgpu::Timer` (timestamp queries): время самого прохода без накладных расходов хоста
- [x] Двумерная сетка групп (`dispatchCovering`), лимиты адаптера вместо умолчаний WebGPU (привязка storage 128 МБ → 2 ГиБ на GTX 1060)
- [x] Постоянный staging у `readBuffer` (было 1,6 мс на вызов, стало ~30 мкс) и чтение в память вызывающего
- [ ] Чтение больших объёмов упирается в копию из отображённой памяти (40 МБ — 13 мс): отдавать отображённую область без копии
- [ ] Загрузка через `copyBuffer` с timestamps внутри encoder — `Transfer.In@dev` отдельно от копии на хосте
- [ ] Алгоритмы по мере надобности: prefix scan, stream compaction (частицы у стенки), reduction (min/max/sum), radix sort и сетка соседей

## CPU

- [x] `Tasks::Pool::forPartitions`; ядра принимают раздел строк `[begin, end)` (`Walls`)
- [ ] Перевести `Integrators`, `ParticleDynamics::SpatialGrid` на раскладку «модель + `cpu`» при переносе соответствующих пунктов

## Позже

- [ ] `libs/cuda` и бэкенды `<Name>.cuda`
