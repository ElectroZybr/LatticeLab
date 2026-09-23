#pragma once

#include <Lattice/Kernel/NodeSystem.hpp>

namespace Lattice {

class Context {
public:
    Blueprints blueprints;
    NodeSystem nodes;
    Context() : nodes(blueprints) {}
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
};

}
