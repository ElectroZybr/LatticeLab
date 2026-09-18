#pragma once

#include "Lattice/Kernel/SubsystemAPI.hpp"

namespace GPU {
class  GPUAPI : public SubsystemAPI {
public:
    virtual ~GPUAPI() = default;
};

}