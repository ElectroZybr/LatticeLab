#pragma once

namespace ParticleDynamics {

/// Plain C++ contracts. Registration and ownership belong to composition.
struct IntegratorAPI {
    virtual ~IntegratorAPI() = default;
    virtual void step() = 0;
};

struct ForceFieldAPI {
    virtual ~ForceFieldAPI() = default;
    virtual bool compute() = 0;
};

struct SpatialIndexAPI {
    virtual ~SpatialIndexAPI() = default;
    virtual void rebuild() = 0;
};

}
