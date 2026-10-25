# WGPU: общее GPU-устройство

Пакет: `compose.cpp` создаёт одно устройство приложения и публикует его. Сами обёртки над
wgpu-native — библиотека [`libs/wgpu`](../../libs/wgpu/README.md).

```text
WGPU
├── Instance:instance
└── Device:<адаптер>           shared → instance; action resources
```

Потребители монтируют устройство (`root.mount<Wgpu::Device>()`) и владеют своими
ресурсами как обычными RAII-объектами; действие `resources` показывает живые ресурсы.
