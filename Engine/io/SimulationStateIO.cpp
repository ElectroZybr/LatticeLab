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

    file << "box " << simulation.sim_box.size.x << " " << simulation.sim_box.size.y << " " << simulation.sim_box.size.z << "\n";
    file << "step " << simulation.sim_step << "\n";
    file << "time_fs " << simulation.sim_time_fs << "\n";
    file << "dt " << simulation.getDt() << "\n";
    file << "integrator " << static_cast<int>(simulation.getIntegrator()) << "\n";
    const Vec3f gravity = simulation.forceField.getGravity();
    file << "gravity " << gravity.x << " " << gravity.y << " " << gravity.z << "\n";
    file << "neighbor_list " << static_cast<int>(simulation.isNeighborListEnabled()) << "\n";
    file << "cell_size " << simulation.sim_box.grid.cellSize << "\n";
    file << "cutoff_nl " << simulation.neighborList.cutoff() << "\n";
    file << "skin_nl " << simulation.neighborList.skin() << "\n";
    file << "max_speed " << simulation.getMaxParticleSpeed() << "\n";
    file << "accel_damping " << simulation.getAccelDamping() << "\n";

    for (size_t atomIndex = 0; atomIndex < simulation.atomStorage.size(); ++atomIndex) {
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

    Vec3f boxSize;
    int cellSize = -1;
    int loadedStep = 0;
    double loadedTimeFs = 0.0;
    float loadedDt = simulation.getDt();
    int loadedIntegrator = static_cast<int>(simulation.getIntegrator());
    Vec3f loadedGravity = simulation.forceField.getGravity();
    bool loadedNeighborListEnabled = simulation.isNeighborListEnabled();
    float loadedMaxSpeed = 0.0f;
    float loadedAccelDamping = simulation.getAccelDamping();

    std::string tag;
    while (file >> tag) {
        if (tag == "box") {
            file >> boxSize.x >> boxSize.y >> boxSize.z;
        } else if (tag == "step") {
            file >> loadedStep;
        } else if (tag == "time_fs") {
            file >> loadedTimeFs;
        } else if (tag == "dt") {
            file >> loadedDt;
        } else if (tag == "integrator") {
            file >> loadedIntegrator;
        } else if (tag == "gravity") {
            file >> loadedGravity.x >> loadedGravity.y >> loadedGravity.z;
        } else if (tag == "neighbor_list") {
            int enabled = 0;
            file >> enabled;
            loadedNeighborListEnabled = (enabled != 0);
        } else if (tag == "cell_size") {
            file >> cellSize;
        } else if (tag == "cutoff_nl") {
            float cutoff = simulation.neighborList.cutoff();
            file >> cutoff;
            simulation.neighborList.setCutoff(cutoff);
        } else if (tag == "skin_nl") {
            float skin = simulation.neighborList.skin();
            file >> skin;
            simulation.neighborList.setSkin(skin);
        } else if (tag == "max_speed") {
            file >> loadedMaxSpeed;
        } else if (tag == "accel_damping") {
            file >> loadedAccelDamping;
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

    simulation.setSizeBox(boxSize, cellSize);
    simulation.setDt(loadedDt);
    simulation.setIntegrator(static_cast<Integrator::Scheme>(loadedIntegrator));
    simulation.forceField.setGravity(loadedGravity);
    simulation.setNeighborListEnabled(loadedNeighborListEnabled);

    for (const LoadedAtomData& data : buffer) {
        simulation.createAtom(data.coords, data.speed, static_cast<AtomData::Type>(data.type), data.fixed);
    }

    simulation.sim_step = loadedStep;
    simulation.sim_time_fs = loadedTimeFs;
    simulation.setMaxParticleSpeed(loadedMaxSpeed);
    simulation.setAccelDamping(loadedAccelDamping);
}

