#pragma once

#include <array>
#include <Lattice/Kernel/NodeViews.hpp>
#include "StdData/include/NamedSoA.hpp"

#include "AtomStorage.hpp"

namespace ClassicMD {

struct Element {using type = std::array<char, 8>;};
struct Mass    {using type = float;};

class AtomData final : public StdData::NamedSoA {
public:
    explicit AtomData(NodeBuild branch) {
        addCol<Element>();
        addCol<Mass>();
        addCol<Valence>();
    }
};

}