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

static DebugView* buildDebugTimersView(DebugPanel& panel) {
    return panel.addView(DebugView("Timers", {
        DebugValue("Интегратор step (avg, мс)", DebugDrawers::Float<4>),
        DebugValue("Интегратор step (max, мс)", DebugDrawers::Float<4>),
        DebugSeries("Интегратор step (last, график)"),
        DebugValue("NL build (avg, мс)", DebugDrawers::Float<4>),
        DebugValue("NL build (max, мс)", DebugDrawers::Float<4>),
        DebugSeries("NL build (last, график)"),
        DebugValue("NL needsRebuild (avg, мс)", DebugDrawers::Float<4>),
        DebugValue("NL needsRebuild (max, мс)", DebugDrawers::Float<4>),
        DebugSeries("NL needsRebuild (last, график)"),
        DebugValue("SG rebuild (avg, мс)", DebugDrawers::Float<4>),
        DebugValue("SG rebuild (max, мс)", DebugDrawers::Float<4>),
        DebugSeries("SG rebuild (last, график)"),
    }));
}
} // namespace

DebugViews createDebugViews(DebugPanel& panel) {
    return DebugViews{
        buildDebugSimView(panel),
        buildDebugAtomSingle(panel),
        buildDebugAtomBatch(panel),
        buildDebugNeighborView(panel),
        buildDebugTimersView(panel),
    };
}


