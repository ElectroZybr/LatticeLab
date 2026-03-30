#include "CreateDebugPanels.h"

#include "GUI/interface/panels/debug/DebugDrawers.h"
#include "GUI/interface/panels/debug/DebugPanel.h"
#include "GUI/interface/panels/debug/view/DebugView.h"

namespace {
static DebugView* buildDebugSimView(DebugPanel& panel) {
    return panel.addView(DebugView("Симуляция", {
        DebugValue ("Память (МБ)", DebugDrawers::Float<2>),
        DebugValue ("Рендер (мс)", DebugDrawers::Float<4>),
        DebugValue ("Физика (мс)", DebugDrawers::Float<4>),
        DebugValue ("Тип интегратора", DebugDrawers::String),
        DebugValue ("Шаги симуляции", DebugDrawers::Int),
        DebugValue ("Шагов/с", DebugDrawers::Float<2>),
        DebugValue ("Количество атомов", DebugDrawers::Int),
        DebugSeries("Полная энергия"),
    }));
}

static DebugView* buildDebugAtomSingle(DebugPanel& panel) {
    return panel.addView(DebugView("Атом", {
        DebugValue("Позиция", DebugDrawers::Vec3f<3>),
        DebugValue("Скорость", DebugDrawers::Vec3f<3>),
        DebugValue("Силы", DebugDrawers::Vec3f<3>),
        DebugValue("Пред. силы", DebugDrawers::Vec3f<3>),
        DebugValue("Потенциальная энергия", DebugDrawers::Float<4>),
        DebugValue("Масса", DebugDrawers::Float<3>),
        DebugValue("Радиус", DebugDrawers::Float<3>),
        DebugValue("Тип", DebugDrawers::Int),
    }));
}

static DebugView* buildDebugAtomBatch(DebugPanel& panel) {
    return panel.addView(DebugView("Атомы", {
        DebugValue("Выбрано атомов", DebugDrawers::Int),
    }));
}

static DebugView* buildDebugNeighborView(DebugPanel& panel) {
    return panel.addView(DebugView("NL", {
        DebugValue("Размер сетки", DebugDrawers::String),
        DebugValue("Размер ячейки", DebugDrawers::Int),
        DebugValue("NeighborList включен", DebugDrawers::String),
        DebugValue("Память AtomStorage (МБ)", DebugDrawers::Float<3>),
        DebugValue("Память NeighborList (МБ)", DebugDrawers::Float<3>),
        DebugValue("Память SpatialGrid (МБ)", DebugDrawers::Float<3>),
        DebugValue("Пар в NL", DebugDrawers::Int),
        DebugValue("Ср. соседей на атом", DebugDrawers::Float<3>),
        DebugValue("Cutoff", DebugDrawers::Float<3>),
        DebugValue("Skin", DebugDrawers::Float<3>),
        DebugValue("List radius", DebugDrawers::Float<3>),
        DebugValue("Ребилдов NL", DebugDrawers::Int),
        DebugValue("Шагов между ребилдами (recent)", DebugDrawers::Float<2>),
        DebugValue("Время ребилда NL (мс)", DebugDrawers::Float<4>),
        DebugValue("Время needsRebuild (мс)", DebugDrawers::Float<4>),
        DebugValue("SG заполненных ячеек", DebugDrawers::Int),
        DebugValue("SG макс атомов в ячейке", DebugDrawers::Int),
        DebugValue("SG ср. атомов/ячейку", DebugDrawers::Float<3>),
    }));
}

static DebugView* buildDebugProfilerView(DebugPanel& panel) {
    return panel.addView(DebugView("Profiler", {
        DebugValue("Кадр (мс)", DebugDrawers::Float<4>),
        DebugValue("Tracked (мс)", DebugDrawers::Float<4>),
        DebugValue("Application::PhysicsStep (мс)", DebugDrawers::Float<4>),
        DebugValue("Application::PhysicsStep (%)", DebugDrawers::Float<2>),
        DebugValue("Application::RenderFrame (мс)", DebugDrawers::Float<4>),
        DebugValue("Application::RenderFrame (%)", DebugDrawers::Float<2>),
        DebugValue("Simulation::update (мс)", DebugDrawers::Float<4>),
        DebugValue("Simulation::update (%)", DebugDrawers::Float<2>),
        DebugValue("ForceField::compute (мс)", DebugDrawers::Float<4>),
        DebugValue("ForceField::compute (%)", DebugDrawers::Float<2>),
        DebugValue("NeighborList::build (мс)", DebugDrawers::Float<4>),
        DebugValue("NeighborList::build (%)", DebugDrawers::Float<2>),
        DebugValue("NeighborList::needsRebuild (мс)", DebugDrawers::Float<4>),
        DebugValue("NeighborList::needsRebuild (%)", DebugDrawers::Float<2>),
        DebugValue("SpatialGrid::rebuild (мс)", DebugDrawers::Float<4>),
        DebugValue("SpatialGrid::rebuild (%)", DebugDrawers::Float<2>),
        DebugValue("RendererGL::drawShot (мс)", DebugDrawers::Float<4>),
        DebugValue("RendererGL::drawShot (%)", DebugDrawers::Float<2>),
        DebugSeries("Frame (график)"),
        DebugSeries("Simulation::update (график)"),
        DebugSeries("ForceField::compute (график)"),
        DebugSeries("NeighborList::build (график)"),
        DebugSeries("RendererGL::drawShot (график)"),
    }));
}
} // namespace

DebugViews createDebugViews(DebugPanel& panel) {
    return DebugViews{
        buildDebugSimView(panel),
        buildDebugAtomSingle(panel),
        buildDebugAtomBatch(panel),
        buildDebugNeighborView(panel),
        buildDebugProfilerView(panel),
    };
}


