#pragma once

#include "Lattice/Kernel/SubsystemAPI.hpp"

enum class DevicePreference {
    Any,
    Discrete,
    Integrated
};

struct GPUDesc {
    DevicePreference preference = DevicePreference::Any;
};

namespace GPU {
class  GPUAPI : public SubsystemAPI {
public:
    using Desc = GPUDesc;
    virtual ~GPUAPI() = default;
};

}