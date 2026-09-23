#pragma once

#include <Lattice/Kernel/Consts.hpp>

namespace ParticleDynamics {
struct IntegratorAPI : public Lattice::Component {
    virtual void step() = 0;
};

struct ForceFieldAPI : public Lattice::Component {
    virtual bool compute() = 0;
};

struct SpatialIndexAPI : public Lattice::Component {
    virtual void rebuild() = 0;
};
}
