#include <algorithm>

#include <Lattice/Kernel/Objects.hpp>


namespace Lattice {

Path::Path(ObjectId id, Objects& objectBlueprints) {
    while (Objects::valid(id)) {
        ids_.push_back(id);
        id = objectBlueprints.require(id).parent;
    }

    std::ranges::reverse(ids_);
}

}