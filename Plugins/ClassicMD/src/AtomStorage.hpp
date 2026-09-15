#pragma once

#include "ParticleDynamics/include/ParticleStorage.hpp"

namespace ClassicMD {

struct DataId  {using type = uint32_t;};
struct Energy  {using type = float;};
struct Charge  {using type = float;};
struct Valence {using type = uint8_t;};

class AtomStorage final : public ParticleDynamics::ParticleStorage {
public:
    explicit AtomStorage(Lattice::Node& branch)
        : ParticleStorage(branch) {
        addCol<DataId>();
        addCol<Energy>();
        addCol<Charge>();
        addCol<Valence>();
    }
};

} // namespace ClassicMD