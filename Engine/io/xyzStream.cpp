#include "xyzStream.h"

#include "Engine/Simulation.h"

#include <fstream>
#include <iostream>

void xyzStream::Start(std::string path) {
    path_ = path + "/stream.xyz";
    std::ofstream file(path_, std::ios::trunc);
    if (!file.is_open()) {
        return;
    }
    isStreaming = true;
}

void xyzStream::WriteFrame(const Simulation& simulation) {
    std::cerr << "Frame written to " << path_ << std::endl;
    std::ofstream file(path_.data(), std::ios::app);
    if (!file.is_open()) {
        return;
    }

    const size_t atomCount = simulation.atoms().size();
    file << atomCount << "\n";
    file << "Step: " << simulation.world().getSimStep() << ", Time: " << simulation.world().getSimTimeNs() << " ns\n";
    for (size_t i = 0; i < atomCount; ++i) {
        const Vec3f pos = simulation.atoms().pos(i);
        file << AtomData::symbol(simulation.atoms().type(i)) << " " << pos.x << " " << pos.y << " " << pos.z << "\n";
    }
}

void xyzStream::Stop() {
    isStreaming = false;
}
