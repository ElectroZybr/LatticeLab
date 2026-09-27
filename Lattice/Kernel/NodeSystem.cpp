#include <Lattice/Kernel/NodeSystem.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

namespace Lattice {

::NodeConfigure NodeSystem::configure(NodeId id) {
    registry.require(id);
    return ::NodeConfigure{id, *this};
}

}
