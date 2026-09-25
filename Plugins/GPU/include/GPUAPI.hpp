#pragma once

#include <Lattice/Kernel/Consts.hpp>

namespace GPU {

enum class DevicePreference {
    Any,
    Discrete,
    Integrated
};

struct GPUDesc {
    DevicePreference preference = DevicePreference::Any;
};

class GPUAPI : public Lattice::SubsystemAPI {
public:
    using Desc = GPUDesc;
    virtual ~GPUAPI() = default;
};

}
