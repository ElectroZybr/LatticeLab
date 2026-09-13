#pragma once

#include <array>

#include <Lattice/Kernel/Node.hpp>
#include "StdData/include/SoA.hpp"

namespace ClassicMD {

struct Name { using type = std::array<char, 8>; };
struct Mass { using type = float; };
struct Valence { using type = uint8_t; };

class AtomData {
public:
    explicit AtomData(Lattice::Node& branch) {
        branch.add<StdData::SoA>();
        soa_ = branch.require<StdData::SoA>();

        soa_->addCol<Name>();
        soa_->addCol<Mass>();
        soa_->addCol<Valence>();
    }

private:
    Ref<StdData::SoA> soa_;
};

} // namespace ClassicMD