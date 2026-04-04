#include "UpdateDebugData.h"
#include <algorithm>

#include <string>

#include "CreateDebugPanels.h"
#include "Engine/metrics/EnergyMetrics.h"
#include "Engine/metrics/MemoryMetrics.h"
#include "Engine/metrics/Profiler.h"
#include "Engine/Simulation.h"
#include "Engine/Consts.h"
#include "App/interaction/ToolsManager.h"
#include "GUI/interface/panels/debug/view/DebugView.h"

void updateAtomSelectionDebug(const DebugViews& debugViews, const Simulation& simulation) {
    if (ToolsManager::pickingSystem->getSelectedIndices().size() == 1)
    {
        debugViews.atomSingle->visible = true;
        debugViews.atomBatch->visible = false;
        const size_t selectedIndex = *ToolsManager::pickingSystem->getSelectedIndices().begin();
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
        debugViews.atomBatch->add_data("Выбрано атомов", ToolsManager::pickingSystem->getSelectedIndices().size());
    }
}

void updateSimulationDebug(const DebugViews& debugViews, const Simulation& simulation,
                           std::string_view integratorName) {
    const Profiler& profiler = Profiler::instance();
    const double renderMs = profiler.lastMs("Application::RenderFrame");
    const double physicsMs = profiler.lastActiveMs("Simulation::update");
    const double nlNeedsRebuildMs = profiler.lastMs("NeighborList::needsRebuild");
    const float stepsPerSecond = static_cast<float>(profiler.counterRate("Simulation::steps"));

    debugViews.sim->add_data("Полная энергия (pj)", simulation.fullEnegryPJ());
    debugViews.sim->add_data("Полная средняя энергия (eV)", static_cast<float>(EnergyMetrics::fullAverageEnergy(simulation.atomStorage)));
    debugViews.sim->add_data("Память (МБ)", MemoryMetrics::getRSS() / 1024.f / 1024.f);
    debugViews.sim->add_data("Рендер (мс)", renderMs);
    debugViews.sim->add_data("Физика (мс)", physicsMs);
    debugViews.sim->add_data("Количество атомов", simulation.atomStorage.size());
    debugViews.sim->add_data("Шаги симуляции", simulation.getSimStep());
    debugViews.sim->add_data("Шагов/с", stepsPerSecond);
    debugViews.sim->add_data("Время симуляции (ns)", simulation.simTimeNs());
    debugViews.sim->add_data("Тип интегратора", integratorName);

    const std::string gridSize = std::to_string(std::max(0, simulation.sim_box.grid.sizeX - 2))
        + " x " + std::to_string(std::max(0, simulation.sim_box.grid.sizeY - 2))
        + " x " + std::to_string(std::max(0, simulation.sim_box.grid.sizeZ - 2));
    debugViews.neighbor->add_data("Размер сетки", gridSize);
    const std::string boxSizeNm =
        std::format("{:.2f} x {:.2f} x {:.2f}",
            simulation.sim_box.size.x * Units::AngstromToNm,
            simulation.sim_box.size.y * Units::AngstromToNm,
            simulation.sim_box.size.z * Units::AngstromToNm);
    debugViews.neighbor->add_data("Размер бокса (nm)", boxSizeNm);
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
    debugViews.neighbor->add_data("Ребилдов NL", simulation.neighborList.stats().rebuildCount());
    debugViews.neighbor->add_data("Шагов между ребилдами (recent)", simulation.neighborList.stats().recentAverageStepsBetweenRebuilds());
    debugViews.neighbor->add_data("Время ребилда NL (мс)", simulation.neighborList.stats().lastRebuildTimeMs());
    debugViews.neighbor->add_data("Время needsRebuild (мс)", nlNeedsRebuildMs);
    debugViews.neighbor->add_data("SG заполненных ячеек",
        static_cast<int>(simulation.sim_box.grid.stats().lastNonEmptyCellCount()));
    debugViews.neighbor->add_data("SG макс атомов в ячейке",
        static_cast<int>(simulation.sim_box.grid.stats().lastMaxAtomsPerCell()));
    debugViews.neighbor->add_data("SG ср. атомов/ячейку",
        simulation.sim_box.grid.stats().lastAverageAtomsPerNonEmptyCell());

}