#include "MemoryTraffic.hpp"

#include <memory>

#include "AmdDfMemoryTraffic.hpp"

namespace Lattice::Benchmarks {

MemoryTraffic::MemoryTraffic()
    : MemoryTraffic(std::make_unique<AmdDfMemoryTraffic>()) {}

}
