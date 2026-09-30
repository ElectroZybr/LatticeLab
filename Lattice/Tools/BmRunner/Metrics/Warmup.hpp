#pragma once

#include <string_view>

#include <Lattice/Tools/BmRunner/Stages.hpp>

namespace Lattice::Benchmarks {

class Warmup : public Capability {
public:
    std::string_view name() const noexcept override {
        return "Warmup";
    }
};

}