#pragma once

#include <array>
#include <stddata/NamedSoA.hpp>

#include "AtomStorage.hpp"

namespace ClassicMD {

struct Element {using type = std::array<char, 8>;};
struct Mass    {using type = float;};

class AtomData final : public StdData::NamedSoA {
public:
    AtomData() {
        addCol<Element>();
        addCol<Mass>();
        addCol<Valence>();
    }
};

}
