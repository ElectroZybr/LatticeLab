#include <Lattice/Composition.hpp>
#include <Actions/ActionRouter.hpp>

extern "C" constinit const Lattice::Package lattice_package{
    .id = "Actions",
    .version = {1, 0, 0},
};

// One input router for the application. Branches that own input devices mount it,
// register their devices and call tick() from their input loop.
extern "C" void compose(NodeBuild root) {
    root.emplaceShared<ActionRouter>("router", root.exports(), root.id());
}
