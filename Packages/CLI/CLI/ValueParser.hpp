#pragma once

#include <optional>
#include <string_view>

#include <Lattice/Kernel/Value.hpp>
#include "Lattice/Kernel/Consts.hpp"
#include "Lattice/Kernel/TreeView.hpp"

namespace CLIPlugin {

std::string_view trim(std::string_view value);
std::optional<Lattice::Value> parseValue(
    std::string_view source,
    const Lattice::Value& current
);

Lattice::NodeId resolveTreePath(
    const Lattice::TreeView& tree,
    Lattice::NodeId current,
    std::string_view expression
);

std::optional<std::vector<std::string>> parseArguments(std::string_view source);

}
