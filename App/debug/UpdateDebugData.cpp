#include "UpdateDebugData.h"

#include <string>

#include "CreateDebugPanels.h"
#include "Engine/metrics/MemoryMetrics.h"
#include "Engine/Simulation.h"
#include "Engine/tools/Tools.h"
#include "GUI/interface/panels/debug/view/DebugView.h"

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

    const std::string gridSize = std::to_string(static_cast<int>(simulation.sim_box.grid.sizeX/simulation.sim_box.grid.cellSize))
        + " x " + std::to_string(static_cast<int>(simulation.sim_box.grid.sizeY/simulation.sim_box.grid.cellSize))
        + " x " + std::to_string(static_cast<int>(simulation.sim_box.grid.sizeZ/simulation.sim_box.grid.cellSize));
    debugViews.neighbor->add_data("Размер сетки", gridSize);
    debugViews.neighbor->add_data("Размер ячейки", simulation.sim_box.grid.cellSize);
    debugViews.neighbor->add_data("NeighborList включен",
        simulation.isNeighborListEnabled() ? std::string("Да") : std::string("Нет"));
    debugViews.neighbor->add_data("Память AtomStorage (МБ)", static_cast<float>(simulation.atomStorage.memoryBytes()) / 1024.0f / 1024.0f);
    debugViews.neighbor->add_data("Память NeighborList (МБ)", static_cast<float>(simulation.neighborList.memoryBytes()) / 1024.0f / 1024.0f);
    debugViews.neighbor->add_data("Пар в NL", simulation.neighborList.pairStorageSize());
    debugViews.neighbor->add_data("Cutoff", simulation.neighborList.cutoff());
    debugViews.neighbor->add_data("Skin", simulation.neighborList.skin());
    debugViews.neighbor->add_data("List radius", simulation.neighborList.listRadius());
    debugViews.neighbor->add_data("Ребилдов NL", simulation.neighborListRebuildCount());
    debugViews.neighbor->add_data("Шагов между ребилдами (recent)", simulation.recentAverageStepsPerNeighborListRebuild());
    debugViews.neighbor->add_data("Время ребилда NL (last, мс)", simulation.lastNeighborListRebuildTimeMs());
    debugViews.neighbor->add_data("Время ребилда NL (avg, мс)", simulation.averageNeighborListRebuildTimeMs());
    debugViews.neighbor->add_data("Время ребилда NL (max, мс)", simulation.maxNeighborListRebuildTimeMs());
}
