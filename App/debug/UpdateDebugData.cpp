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
    const AtomStorage& atoms = simulation.atoms();
    if (ToolsManager::pickingSystem->getSelectedIndices().size() == 1)
    {
        debugViews.atomSingle->visible = true;
        debugViews.atomBatch->visible = false;
        const size_t selectedIndex = *ToolsManager::pickingSystem->getSelectedIndices().begin();
        if (selectedIndex < atoms.size()) {
            debugViews.atomSingle->add_data("Позиция", atoms.pos(selectedIndex));
            debugViews.atomSingle->add_data("Скорость", atoms.vel(selectedIndex));
            debugViews.atomSingle->add_data("Силы", atoms.force(selectedIndex));
            debugViews.atomSingle->add_data("Пред. силы", atoms.prevForce(selectedIndex));
            debugViews.atomSingle->add_data("Потенциальная энергия", atoms.energy(selectedIndex));
            const AtomData::Type atomType = atoms.type(selectedIndex);
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
    const AtomStorage& atoms = simulation.atoms();
    const SimBox& box = simulation.box();
    const NeighborList& neighborList = simulation.neighborList();
    const Profiler& profiler = Profiler::instance();
    const double renderMs = profiler.lastMs("Application::RenderFrame");
    const double physicsMs = profiler.lastActiveMs("Simulation::update");
    const double nlNeedsRebuildMs = profiler.lastMs("NeighborList::needsRebuild");
    const float stepsPerSecond = static_cast<float>(profiler.counterRate("Simulation::steps"));

    debugViews.sim->add_data("Полная энергия (pj)", simulation.fullEnegryPJ());
    debugViews.sim->add_data("Полная средняя энергия (eV)", static_cast<float>(EnergyMetrics::fullAverageEnergy(atoms)));
    debugViews.sim->add_data("Память (МБ)", MemoryMetrics::getRSS() / 1024.f / 1024.f);
    debugViews.sim->add_data("Рендер (мс)", renderMs);
    debugViews.sim->add_data("Физика (мс)", physicsMs);
    debugViews.sim->add_data("Количество атомов", atoms.size());
    debugViews.sim->add_data("Шаги симуляции", simulation.getSimStep());
    debugViews.sim->add_data("Шагов/с", stepsPerSecond);
    debugViews.sim->add_data("Время симуляции (ns)", simulation.simTimeNs());
    debugViews.sim->add_data("Тип интегратора", integratorName);

    const std::string gridSize = std::to_string(std::max(0, box.grid.sizeX - 2))
        + " x " + std::to_string(std::max(0, box.grid.sizeY - 2))
        + " x " + std::to_string(std::max(0, box.grid.sizeZ - 2));
    debugViews.neighbor->add_data("Размер сетки", gridSize);
    const std::string boxSizeNm =
        std::format("{:.2f} x {:.2f} x {:.2f}",
            box.size.x * Units::AngstromToNm,
            box.size.y * Units::AngstromToNm,
            box.size.z * Units::AngstromToNm);
    debugViews.neighbor->add_data("Размер бокса (nm)", boxSizeNm);
    debugViews.neighbor->add_data("Размер ячейки", box.grid.cellSize);
    debugViews.neighbor->add_data("NeighborList включен",
        simulation.isNeighborListEnabled() ? std::string("Да") : std::string("Нет"));
    debugViews.neighbor->add_data("Память AtomStorage (МБ)", static_cast<float>(atoms.memoryBytes()) / 1024.0f / 1024.0f);
    debugViews.neighbor->add_data("Память NeighborList (МБ)", static_cast<float>(neighborList.memoryBytes()) / 1024.0f / 1024.0f);
    debugViews.neighbor->add_data("Память SpatialGrid (МБ)", static_cast<float>(box.grid.memoryBytes()) / 1024.0f / 1024.0f);
    debugViews.neighbor->add_data("Пар в NL", neighborList.pairStorageSize());
    const float avgNeighborsPerAtom = neighborList.atomCount() > 0
        ? (2.0f * static_cast<float>(neighborList.pairStorageSize()))
            / static_cast<float>(neighborList.atomCount())
        : 0.0f;
    debugViews.neighbor->add_data("Ср. соседей на атом", avgNeighborsPerAtom);
    debugViews.neighbor->add_data("Cutoff", neighborList.cutoff());
    debugViews.neighbor->add_data("Skin", neighborList.skin());
    debugViews.neighbor->add_data("List radius", neighborList.listRadius());
    debugViews.neighbor->add_data("Ребилдов NL", neighborList.stats().rebuildCount());
    debugViews.neighbor->add_data("Шагов между ребилдами (recent)", neighborList.stats().recentAverageStepsBetweenRebuilds());
    debugViews.neighbor->add_data("Время ребилда NL (мс)", neighborList.stats().lastRebuildTimeMs());
    debugViews.neighbor->add_data("Время needsRebuild (мс)", nlNeedsRebuildMs);
    debugViews.neighbor->add_data("SG заполненных ячеек", static_cast<int>(box.grid.stats().lastNonEmptyCellCount()));
    debugViews.neighbor->add_data("SG макс атомов в ячейке", static_cast<int>(box.grid.stats().lastMaxAtomsPerCell()));
    debugViews.neighbor->add_data("SG ср. атомов/ячейку", box.grid.stats().lastAverageAtomsPerNonEmptyCell());

}