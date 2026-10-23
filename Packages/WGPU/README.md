# WGPU: GPU-бэкенд

Единственный GPU-бэкенд, без промежуточной абстракции. Пакет `Packages/WGPU` содержит RAII-обёртки над wgpu-native и композицию,
публикующую общее устройство. Цели: `WGPU.core` — реализация без линковки runtime,
`WGPU` — загружаемый пакет, `WGPU.tests` — тесты реализации:

```text
WGPU
├── Instance:instance
└── Device:device              shared → instance; action resources
```

Ресурсы — обычные объекты с конструктором от `Device&`; ими владеют компоненты:

```cpp
auto device = root.mount<WGPU::Device>();
// внутри компонента:
WGPU::Buffer uniform{device, {.size = 64, .usage = WGPU::BufferUsage::Uniform, .label = "uniform"}};
WGPU::Shader shader{device, {.source = wgsl, .label = "scene"}};
WGPU::Pipeline pipeline{device, shader, {.label = "scene"}};
```

Каждый ресурс регистрируется в `device.resources()` и снимается в деструкторе.
`resources` в CLI показывает живые ресурсы; если при уничтожении устройства что-то
осталось, в лог пишется список. Компонент, владеющий ресурсами, зависит от устройства,
поэтому уничтожается раньше него.

```sh
xmake build WGPU.tests && xmake run WGPU.tests   # проверки устройства — если есть адаптер
```

```toml
[Startup]
WGPU = "on"   # до веток, которые монтируют устройство
```
