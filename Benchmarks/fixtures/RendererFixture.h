#pragma once

#include <cmath>
#include <memory>

#include <benchmark/benchmark.h>

#include "App/interaction/ToolsManager.h"
#include "App/interaction/picking/PickingSystem.h"
#include "Engine/SimBox.h"
#include "Engine/physics/AtomData.h"
#include "Engine/physics/AtomStorage.h"
#include "Engine/physics/Bond.h"
#include "Rendering/BaseRenderer.h"
#include "Rendering/BgfxContext.h"

template <typename T>
concept IsRenderer = std::derived_from<T, IRenderer>;

template <IsRenderer TRenderer> class RendererFixture : public benchmark::Fixture {
public:
    RendererFixture() { BgfxContext::instance().init(nullptr, 800, 600); }

    void SetUp(benchmark::State& state) override {
        atomStorage_ = makeGridAtoms(state.range(0));

        renderer_ = std::make_unique<TRenderer>(box_);
        renderer_->camera.setScreenSize({800.0f, 600.0f});

        if (ToolsManager::pickingSystem) {
            delete ToolsManager::pickingSystem;
        }
        ToolsManager::pickingSystem = new PickingSystem(atomStorage_, box_, renderer_);
    }

    void TearDown(benchmark::State&) override {
        renderer_.reset();
        bgfx::frame();
    }

protected:
    void setCounters(benchmark::State& state) const {
        state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(atomStorage_.size()));
    }

    std::unique_ptr<IRenderer> renderer_;
    AtomStorage atomStorage_;
    Bond::List bonds_;
    SimBox box_{Vec3f(300, 300, 300)};

private:
    static AtomStorage makeGridAtoms(int count) {
        AtomStorage atoms;
        atoms.reserve(count);
        const int side = static_cast<int>(std::cbrt(count)) + 1;
        for (int i = 0; i < count; ++i) {
            atoms.addAtom(Vec3f((i % side) * 3.0, ((i / side) % side) * 3.0, (i / static_cast<double>(side * side)) * 3.0),
                          Vec3f::Random() * 0.5, AtomData::Type::H);
        }
        return atoms;
    }
};
