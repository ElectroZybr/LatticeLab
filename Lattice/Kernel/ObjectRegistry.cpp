#include <algorithm>

#include <Lattice/Kernel/ObjectRegistry.hpp>


namespace Lattice {

Path::Path(ObjectId id, ObjectRegistry& objectRegistry) {
    while (valid(id)) {
        ids_.push_back(id);
        id = objectRegistry.require(id).parent;
    }

    std::ranges::reverse(ids_);
}

}