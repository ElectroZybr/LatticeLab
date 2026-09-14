#pragma once

#include <array>

#include <Lattice/Kernel/Node.hpp>
#include "StdData/include/NamedSoA.hpp"

namespace ClassicMD {

struct Name { using type = std::array<char, 8>; };
struct Mass { using type = float; };
struct Valence { using type = uint8_t; };

class AtomData {
public:
    explicit AtomData(Lattice::Node& branch) {
        branch.add<StdData::NamedSoA>();
        soa_ = branch.require<StdData::NamedSoA>();

        soa_->addCol<Name>();
        soa_->addCol<Mass>();
        soa_->addCol<Valence>();
    }

private:
    Ref<StdData::NamedSoA> soa_;
};

} // namespace ClassicMD