#include "Simulation.h"

#include <algorithm>
#include <stdexcept>

#include "Engine/metrics/Profiler.h"
#include "Engine/io/SimulationStateIO.h"

Simulation::Simulation() {
    xyzStreamer_.Start(".");
}

Simulation::WorldId Simulation::createWorld(Vec3f size, Vec3f renderOffset) {
    worlds_.emplace_back(size, renderOffset);
    const WorldId worldId = worlds_.size() - 1;
    if (worlds_.size() == 1) {
        activeWorldIndex_ = worldId;
    }
    return worldId;
}

bool Simulation::removeWorld(WorldId worldId) {
    if (worldId >= worlds_.size() || worlds_.size() <= 1) {
        return false;
    }

    worlds_.erase(worlds_.begin() + static_cast<std::ptrdiff_t>(worldId));
    if (activeWorldIndex_ == worldId) {
        activeWorldIndex_ = std::min(worldId, worlds_.size() - 1);
    }
    else if (activeWorldIndex_ > worldId) {
        --activeWorldIndex_;
    }
    return true;
}

bool Simulation::setActiveWorld(WorldId worldId) {
    if (worldId >= worlds_.size()) {
        return false;
    }
    activeWorldIndex_ = worldId;
    return true;
}

World& Simulation::worldAt(WorldId worldId) {
    if (worldId >= worlds_.size()) {
        throw std::out_of_range("Simulation::worldAt: invalid world id");
    }
    return worlds_[worldId];
}

const World& Simulation::worldAt(WorldId worldId) const {
    if (worldId >= worlds_.size()) {
        throw std::out_of_range("Simulation::worldAt: invalid world id");
    }
    return worlds_[worldId];
}

World& Simulation::world() {
    if (worlds_.empty() || activeWorldIndex_ >= worlds_.size()) {
        throw std::runtime_error("Simulation: no active world");
    }
    return worlds_[activeWorldIndex_];
}

const World& Simulation::world() const {
    if (worlds_.empty() || activeWorldIndex_ >= worlds_.size()) {
        throw std::runtime_error("Simulation: no active world");
    }
    return worlds_[activeWorldIndex_];
}

void Simulation::update() {
    PROFILE_SCOPE("Simulation::update");
    world().update();
}

void Simulation::updateWorld(WorldId worldId) {
    if (worldId < worlds_.size()) {
        worlds_[worldId].update();
    }
}

void Simulation::updateAll() {
    PROFILE_SCOPE("Simulation::updateAll");
    xyzStreamer_.WriteFrame(*this);
    for (auto& w : worlds_) {
        w.update();
    }
}

void Simulation::setSizeBox(Vec3f newSize, int cellSize) {
    world().resizeBox(newSize, static_cast<float>(cellSize));
}

void Simulation::createAtom(Vec3f start_coords, Vec3f start_speed, AtomData::Type type, bool fixed) {
    world().addAtom(start_coords, start_speed, type, fixed);
}

void Simulation::removeAtom(size_t atomIndex) {
    world().removeAtom(atomIndex);
}

void Simulation::addBond(size_t aIndex, size_t bIndex) { world().addBond(aIndex, bIndex); }

void Simulation::clear() { world().reset(); }
