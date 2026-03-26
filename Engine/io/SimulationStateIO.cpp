#include "SimulationStateIO.h"

#include <fstream>
#include <sstream>
#include <vector>

#include "../Simulation.h"

void SimulationStateIO::save(const Simulation& simulation, std::string_view path) {
    std::ofstream file(path.data());
    if (!file.is_open()) {
        return;
    }

    file << "box "
         << simulation.sim_box.start.x << " " << simulation.sim_box.start.y << " " << simulation.sim_box.start.z << " "
         << simulation.sim_box.end.x   << " " << simulation.sim_box.end.y   << " " << simulation.sim_box.end.z   << "\n";

    file << "step " << simulation.sim_step << "\n";

    for (std::size_t atomIndex = 0; atomIndex < simulation.atomStorage.size(); ++atomIndex) {
        const Vec3f pos = simulation.atomStorage.pos(atomIndex);
        const Vec3f vel = simulation.atomStorage.vel(atomIndex);
        file << "atom "
             << pos.x << " " << pos.y << " " << pos.z << " "
             << vel.x << " " << vel.y << " " << vel.z << " "
             << static_cast<int>(simulation.atomStorage.type(atomIndex)) << " "
             << simulation.atomStorage.isAtomFixed(atomIndex) << "\n";
    }
}

void SimulationStateIO::load(Simulation& simulation, std::string_view path) {
    std::ifstream file(path.data());
    if (!file.is_open()) {
        return;
    }

    simulation.clear();

    struct LoadedAtomData {
        Vec3f coords, speed;
        int type;
        bool fixed;
    };
    std::vector<LoadedAtomData> buffer;

    Vec3f boxStart, boxEnd;
    int cellSize = -1;
    int loadedStep = 0;

    std::string tag;
    while (file >> tag) {
        if (tag == "box") {
            file >> boxStart.x >> boxStart.y >> boxStart.z
                 >> boxEnd.x   >> boxEnd.y   >> boxEnd.z;
        } else if (tag == "step") {
            file >> loadedStep;
        } else if (tag == "atom") {
            LoadedAtomData data{Vec3f(0.f, 0.f, 0.f), Vec3f(0.f, 0.f, 0.f), 0, false};
            std::string atomLine;
            std::getline(file, atomLine);
            std::istringstream atomStream(atomLine);

            if (!(atomStream >> data.coords.x >> data.coords.y >> data.coords.z
                             >> data.speed.x  >> data.speed.y  >> data.speed.z
                             >> data.type)) {
                continue;
            }

            std::vector<double> tail;
            double value = 0.0;
            while (atomStream >> value) {
                tail.push_back(value);
            }
            data.fixed = !tail.empty() && (tail.back() != 0.0);
            buffer.emplace_back(data);
        }
    }

    simulation.setSizeBox(boxStart, boxEnd, cellSize);

    for (const LoadedAtomData& data : buffer) {
        simulation.createAtom(data.coords, data.speed, static_cast<AtomData::Type>(data.type), data.fixed);
    }

    simulation.sim_step = loadedStep;
}

