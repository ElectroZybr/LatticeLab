#pragma once

// #include <span>
#include <string_view>

#include "Lattice/Kernel/SubsystemAPI.hpp"

// #include "Device.hpp"

namespace GPU {
class  GPUAPI : public SubsystemAPI {
public:
    static constexpr std::string_view tag = "GPUAPI";

    // virtual std::span<Device*> devices() const = 0;
    // virtual Device& device(std::size_t index) = 0;

    virtual ~GPUAPI() = default;
};

}