#pragma once

#include <optional>
#include <string_view>

#include <Lattice/kernel/Value.hpp>
#include "Lattice/kernel/Consts.hpp"
#include "Lattice/kernel/TreeView.hpp"

namespace Console {

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
