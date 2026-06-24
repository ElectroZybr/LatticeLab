#include <iostream>

#include "Lattice/Plugins/ClassicMD/ClassicMDPlugin.h"

void runNeighborSearchTests();
void runPhysicsTests();
void runIoTests();

int main() {
    registerClassicMDPlugin();
    runNeighborSearchTests();
    runPhysicsTests();
    runIoTests();
    std::cout << "All Lattice tests passed." << std::endl;
    return 0;
}
