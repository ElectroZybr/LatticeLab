#include <Lattice/Kernel/NodeSystem.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

namespace Lattice {

::NodeBuild NodeSystem::build(NodeId id) {
    registry.require(id);
    return ::NodeBuild{id, *this};
}

::NodeConfigure NodeSystem::configure(NodeId id) {
    registry.require(id);
    return ::NodeConfigure{id, *this};
}

}