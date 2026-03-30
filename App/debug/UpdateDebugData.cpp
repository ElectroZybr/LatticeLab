#include "UpdateDebugData.h"
#include <algorithm>

#include <string>

#include "CreateDebugPanels.h"
#include "Engine/metrics/MemoryMetrics.h"
#include "Engine/metrics/Profiler.h"
#include "Engine/Simulation.h"
#include "Engine/tools/Tools.h"
#include "GUI/interface/panels/debug/view/DebugView.h"

namespace {
const ProfileEntry* findProfileEntry(const std::vector<ProfileEntry>& entries, const char* name) {
    for (const ProfileEntry& entry : entries) {
        if (entry.name != nullptr && std::string_view(entry.name) == name) {
            return &entry;
        }
    }
    return nullptr;
}

void addProfilerMetric(DebugView* view, const std::vector<ProfileEntry>& entries,
                       const char* entryName, const char* msLabel, const char* percentLabel,
                       const char* graphLabel = nullptr) {
    if (view == nullptr) {
        return;
    }

    const ProfileEntry* entry = findProfileEntry(entries, entryName);
    const double ms = entry != nullptr ? entry->lastMs : 0.0;
    const double percent = entry != nullptr ? entry->percentOfFrame : 0.0;

    view->add_data(msLabel, ms);
    view->add_data(percentLabel, percent);
    if (graphLabel != nullptr) {
        view->add_data(graphLabel, static_cast<float>(ms));
    }
}
}

void updateAtomSelectionDebug(const DebugViews& debugViews, const Simulation& simulation) {
    if (Tools::pickingSystem->getSelectedIndices().size() == 1)
    {
        debugViews.atomSingle->visible = true;
        debugViews.atomBatch->visible = false;
        const std::size_t selectedIndex = *Tools::pickingSystem->getSelectedIndices().begin();
        if (selectedIndex < simulation.atomStorage.size()) {
            debugViews.atomSingle->add_data("Позиция", simulation.atomStorage.pos(selectedIndex));
            debugViews.atomSingle->add_data("Скорость", simulation.atomStorage.vel(selectedIndex));
            debugViews.atomSingle->add_data("Силы", simulation.atomStorage.force(selectedIndex));
            debugViews.atomSingle->add_data("Пред. силы", simulation.atomStorage.prevForce(selectedIndex));
            debugViews.atomSingle->add_data("Потенциальная энергия", simulation.atomStorage.energy(selectedIndex));
            const AtomData::Type atomType = simulation.atomStorage.type(selectedIndex);
            debugViews.atomSingle->add_data("Масса", AtomData::getProps(atomType).mass);
            debugViews.atomSingle->add_data("Радиус", AtomData::getProps(atomType).radius);
            debugViews.atomSingle->add_data("Тип", static_cast<int>(atomType));
        }
    }
    else {
        debugViews.atomBatch->visible = true;
        debugViews.atomSingle->visible = false;
        debugViews.atomBatch->add_data("Выбрано атомов", Tools::pickingSystem->getSelectedIndices().size());
    }
}

void updateSimulationDebug(const DebugViews& debugViews, const Simulation& simulation,
                           float renderMs, float physicsMs, float stepsPerSecond,
                           std::string_view integratorName) {
    debugViews.sim->add_data("Полная энергия", static_cast<float>(simulation.fullAverageEnergy()));
    debugViews.sim->add_data("Память (МБ)", MemoryMetrics::getRSS() / 1024.f / 1024.f);
    debugViews.sim->add_data("Рендер (мс)", renderMs);
    debugViews.sim->add_data("Физика (мс)", physicsMs);
    debugViews.sim->add_data("Количество атомов", simulation.atomStorage.size());
    debugViews.sim->add_data("Шаги симуляции", simulation.getSimStep());
    debugViews.sim->add_data("Шагов/с", stepsPerSecond);
    debugViews.sim->add_data("Тип интегратора", integratorName);

    const std::string gridSize = std::to_string(std::max(0, simulation.sim_box.grid.sizeX - 2))
        + " x " + std::to_string(std::max(0, simulation.sim_box.grid.sizeY - 2))
        + " x " + std::to_string(std::max(0, simulation.sim_box.grid.sizeZ - 2));
    debugViews.neighbor->add_data("Размер сетки", gridSize);
    debugViews.neighbor->add_data("Размер ячейки", simulation.sim_box.grid.cellSize);
    debugViews.neighbor->add_data("NeighborList включен",
        simulation.isNeighborListEnabled() ? std::string("Да") : std::string("Нет"));
    debugViews.neighbor->add_data("Память AtomStorage (МБ)", static_cast<float>(simulation.atomStorage.memoryBytes()) / 1024.0f / 1024.0f);
    debugViews.neighbor->add_data("Память NeighborList (МБ)", static_cast<float>(simulation.neighborList.memoryBytes()) / 1024.0f / 1024.0f);
    debugViews.neighbor->add_data("Память SpatialGrid (МБ)", static_cast<float>(simulation.sim_box.grid.memoryBytes()) / 1024.0f / 1024.0f);
    debugViews.neighbor->add_data("Пар в NL", simulation.neighborList.pairStorageSize());
    const float avgNeighborsPerAtom = simulation.neighborList.atomCount() > 0
        ? (2.0f * static_cast<float>(simulation.neighborList.pairStorageSize()))
            / static_cast<float>(simulation.neighborList.atomCount())
        : 0.0f;
    debugViews.neighbor->add_data("Ср. соседей на атом", avgNeighborsPerAtom);
    debugViews.neighbor->add_data("Cutoff", simulation.neighborList.cutoff());
    debugViews.neighbor->add_data("Skin", simulation.neighborList.skin());
    debugViews.neighbor->add_data("List radius", simulation.neighborList.listRadius());
    debugViews.neighbor->add_data("Ребилдов NL", simulation.neighborListRebuildCount());
    debugViews.neighbor->add_data("Шагов между ребилдами (recent)", simulation.recentAverageStepsPerNeighborListRebuild());
    debugViews.neighbor->add_data("Время ребилда NL (мс)", simulation.lastNeighborListRebuildTimeMs());
    debugViews.neighbor->add_data("Время needsRebuild (мс)", simulation.neighborList.needsRebuildCounter().lastMs());
    debugViews.neighbor->add_data("SG заполненных ячеек",
        static_cast<int>(simulation.sim_box.grid.metrics().lastNonEmptyCellCount()));
    debugViews.neighbor->add_data("SG макс атомов в ячейке",
        static_cast<int>(simulation.sim_box.grid.metrics().lastMaxAtomsPerCell()));
    debugViews.neighbor->add_data("SG ср. атомов/ячейку",
        simulation.sim_box.grid.metrics().lastAverageAtomsPerNonEmptyCell());

    if (debugViews.profiler != nullptr) {
        const Profiler& profiler = Profiler::instance();
        const auto& frame = profiler.frameData();
        const auto& entries = profiler.entries();

        debugViews.profiler->add_data("Кадр (мс)", frame.frameMs);
        debugViews.profiler->add_data("Tracked (мс)", frame.totalTrackedMs);
        debugViews.profiler->add_data("Frame (график)", static_cast<float>(frame.frameMs));

        addProfilerMetric(debugViews.profiler, entries,
                          "Application::PhysicsStep",
                          "Application::PhysicsStep (мс)",
                          "Application::PhysicsStep (%)");
        addProfilerMetric(debugViews.profiler, entries,
                          "Application::RenderFrame",
                          "Application::RenderFrame (мс)",
                          "Application::RenderFrame (%)");
        addProfilerMetric(debugViews.profiler, entries,
                          "Simulation::update",
                          "Simulation::update (мс)",
                          "Simulation::update (%)",
                          "Simulation::update (график)");
        addProfilerMetric(debugViews.profiler, entries,
                          "ForceField::compute",
                          "ForceField::compute (мс)",
                          "ForceField::compute (%)",
                          "ForceField::compute (график)");
        addProfilerMetric(debugViews.profiler, entries,
                          "NeighborList::build",
                          "NeighborList::build (мс)",
                          "NeighborList::build (%)",
                          "NeighborList::build (график)");
        addProfilerMetric(debugViews.profiler, entries,
                          "NeighborList::needsRebuild",
                          "NeighborList::needsRebuild (мс)",
                          "NeighborList::needsRebuild (%)");
        addProfilerMetric(debugViews.profiler, entries,
                          "SpatialGrid::rebuild",
                          "SpatialGrid::rebuild (мс)",
                          "SpatialGrid::rebuild (%)");
        addProfilerMetric(debugViews.profiler, entries,
                          "RendererGL::drawShot",
                          "RendererGL::drawShot (мс)",
                          "RendererGL::drawShot (%)",
                          "RendererGL::drawShot (график)");
    }
}


