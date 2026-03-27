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
        DebugValue ("Интегратор (last, мс)", DebugDrawers::Float<4>),
        DebugValue ("Интегратор (avg, мс)", DebugDrawers::Float<4>),
        DebugValue ("Интегратор (max, мс)", DebugDrawers::Float<4>),
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
    return panel.addView(DebugView("NeighborList", {
        DebugValue("Размер сетки", DebugDrawers::String),
        DebugValue("Размер ячейки", DebugDrawers::Int),
        DebugValue("NeighborList включен", DebugDrawers::String),
        DebugValue("Память AtomStorage (МБ)", DebugDrawers::Float<3>),
        DebugValue("Память NeighborList (МБ)", DebugDrawers::Float<3>),
        DebugValue("Пар в NL", DebugDrawers::Int),
        DebugValue("Ср. соседей на атом", DebugDrawers::Float<3>),
        DebugValue("Cutoff", DebugDrawers::Float<3>),
        DebugValue("Skin", DebugDrawers::Float<3>),
        DebugValue("List radius", DebugDrawers::Float<3>),
        DebugValue("Ребилдов NL", DebugDrawers::Int),
        DebugValue("Шагов между ребилдами (recent)", DebugDrawers::Float<2>),
        DebugValue("Время ребилда NL (last, мс)", DebugDrawers::Float<4>),
        DebugValue("Время ребилда NL (avg, мс)", DebugDrawers::Float<4>),
        DebugValue("Время ребилда NL (max, мс)", DebugDrawers::Float<4>),
        DebugValue("Время needsRebuild (last, мс)", DebugDrawers::Float<4>),
        DebugValue("Время needsRebuild (avg, мс)", DebugDrawers::Float<4>),
        DebugValue("Время needsRebuild (max, мс)", DebugDrawers::Float<4>),
    }));
}
} // namespace

DebugViews createDebugViews(DebugPanel& panel) {
    return DebugViews{
        buildDebugSimView(panel),
        buildDebugAtomSingle(panel),
        buildDebugAtomBatch(panel),
        buildDebugNeighborView(panel),
    };
}


