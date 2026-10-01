#pragma once

#include <memory>

namespace Lattice {

struct Fixture {
    using Factory = std::unique_ptr<Fixture> (*)(size_t);
    virtual ~Fixture() = default;

    // быстрый сброс тестируемого состояния, 
    // внутри пересоздание/очистка/копирование из буфера начального состояния
    // вызывается между итерациями в тестах/бенчмарках
    virtual void prepare() {};
};

}
