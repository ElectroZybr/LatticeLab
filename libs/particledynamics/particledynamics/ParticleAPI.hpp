#pragma once

namespace ParticleDynamics {

/// Plain C++ contracts. Registration and ownership belong to composition.
struct SpatialIndexAPI {
    virtual ~SpatialIndexAPI() = default;
    virtual void rebuild() = 0;
};

}
