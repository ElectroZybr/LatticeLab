// #include "App/Application.h"

// int main() {
//     Application application;
//     return application.run();
// }

#include "App/Scenes.h"
#include "Engine/Simulation.h"

int main() {
    SimBox sim_box(Vec3f(50, 50, 6));
    Simulation sim(sim_box);
    Scenes::crystal(sim, 10, AtomData::Type::Z, true);
    for (int i = 0; i < 5000; ++i) {
        sim.update();
    }

    return 0;
}
