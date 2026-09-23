// Kernel dependences
#include <string_view>
#include "Lattice/Kernel/Consts.hpp"

struct TestAPI final : public Lattice::Component {
    static constexpr std::string_view apiName = "TestAPI";
};
