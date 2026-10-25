# Видимая симуляция

Рамки: вьюпорт общий, содержимое — `SceneObject`; данные для рендера — GPU-таблица
(линейная копия SoA одним `writeBuffer` для CPU-симуляции, сам буфер — для GPU-симуляции);
рендер про модель не знает; сцена собирается кодом, сохраняется через `Serializer`.

## Этапы

- [x] `WGPU::GpuSoA : Lattice::Table` — GPU-зеркало `StdData::SoA` (та же раскладка, загрузка одним `writeBuffer`) и GPU-владеемая таблица (`addColumn`/`resize`); `ColumnView` без данных на хосте не разыменовывается
- [x] WGPU: явные layout'ы bind group (`BindingLayout`, `PipelineDesc::layouts` — общий group 0 камеры), depth (`PipelineDesc::depthFormat`, depth-цель у `beginRenderPass`, view у `Texture`), `draw(vertexCount, instanceCount)`, `copyBuffer`, счётчик ошибок `errorCount()`
- [x] Ядро: `extern "C" configure(NodeConfigure)` у пакета (и у вариантов слотов) — повторяемое связывание с текущим деревом в `maintain`, мягкие зависимости; наборы детей обновляются Lattice, а не вручную. Переведены StdIo (парсеры), Shell (окна, ввод, кадры), Graphics (вьюпорты, контроллеры)
- [x] CLI: терминалы через `configure` (`setTerminals` ждёт конца итерации потока CLI)
- [x] Убрать дублирование механизмов Lattice: `Viewport::controller` и набор вьюпортов `Render` — из `configure`; цель ввода — `Focus<InputTargetAPI>` (ядро: `Focus::set(T*)`, `NodeConfigure::focusId`), роутеру её назначает `configure` Shell; `Terminal::current_` оставлен как позиция навигации
- [x] `Viewport` — один конкретный класс (`ViewportArea` + камера + объекты из `configure`); uniform камеры — bind group 0 вьюпорта; `RenderLayout` — общие форматы и layout группы камеры; `TestViewport` → `TestObject : SceneObject`
- [x] `SceneObject::render(RenderPass&, const ViewState&)`; `Render` владеет depth
- [x] Рендер сфер (`Spheres : SceneObject`): `GpuSoA` + смещения x/y/z + колонка вида (необязательна) + палитра (вид → цвет, радиус); `spheres3d.wgsl` с `frag_depth`
- [x] Границы: GPU-копия в ветке рендера (`GpuSoA` с `CpuSource`), симуляция — `ParticleSystemAPI` без WGPU; экспорты симуляции — через очередь потока-владельца (`Tasks::Queue`, `executeOn` объекта, обычные `param`/`action`/`exportTable`); `configure` будится сменой Context только по прочитанным ролям
- [x] CLI: команды, чтение дерева и экспортов — на host через `onMaintain`; worker обслуживает терминалы, строки и ответы передаются по ID сессии без ожидания host; удаление терминала отменяет запросы
- [x] Ветка рендера собирается кодом (пакет Graphics): `Render` (surface, depth, `RenderLayout` внутри) → общие GPU-данные → вьюпорты-сцены (камера, контроллер, объекты); окно и ввод — в Shell
- [x] Источник атомов — `mount<ParticleSystemAPI>`, GPU-копия в ветке рендера (`GpuSoA` с `CpuSource`), сферы — объект сцены вьюпорта
- [ ] Цвет и радиус по элементу: колонка вида (`DataId`) и палитра из `AtomData` — без знания Graphics о ClassicMD
- [ ] Несколько вьюпортов с одними данными и разными камерами; активный — под курсором
- [ ] Параметры камер/объектов — `param`, сохранение/загрузка через `Serializer`
- [ ] Режимы отрисовки частиц — слот `particles` во вьюпорте (`Graphics::Spheres`, позже `Graphics::BlendedSpheres` — «слипание» через сетку соседей); варианты, различающиеся только шейдером, — один класс с шейдером аргументом; шейдер из файла — отдельный вариант с путём-`param`
- [ ] Шейдеры Graphics (`spheres3d.wgsl` и др.) встроить через bin2c вместо чтения по пути от рабочего каталога
- [ ] Чистка: обновить `packages/graphics/README.md`, `AGENTS.md`
