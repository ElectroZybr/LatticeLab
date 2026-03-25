#pragma once

#include <benchmark/benchmark.h>
#include <memory>
#include <cmath>

#include <SFML/Graphics.hpp>

#include "Engine/physics/AtomData.h"
#include "Engine/physics/AtomStorage.h"
#include "Engine/SimBox.h"

template <typename TRenderer>
class RendererFixture : public benchmark::Fixture {
public:
    void SetUp(benchmark::State& state) override {
        renderTexture_ = std::make_unique<sf::RenderTexture>();
        if (!renderTexture_->resize({800, 600})) {
            state.SkipWithError("RenderTexture resize failed");
            return;
        }

        view_ = renderTexture_->getView();;
        renderer_ = std::make_unique<TRenderer>(*renderTexture_, view_);

        atomStorage_ = makeGridAtoms(static_cast<int>(state.range(0)));
        renderer_->setAtomStorage(&atomStorage_);
    }

    void TearDown(benchmark::State&) override {
        renderer_.reset();
    }

protected:
    void setCounters(benchmark::State& state) const {
        state.SetItemsProcessed(
            state.iterations() * static_cast<int64_t>(atomStorage_.size())
        );
    }

    std::unique_ptr<sf::RenderTexture> renderTexture_;
    std::unique_ptr<TRenderer> renderer_;
    sf::View view_;
    AtomStorage atomStorage_;
    SimBox box_{ Vec3f(0, 0, 0), Vec3f(300, 300, 300) };

private:
    static AtomStorage makeGridAtoms(int count) {
        AtomStorage atoms;
        atoms.reserve(count);
        const int side = static_cast<int>(std::cbrt(count)) + 1;
        for (int i = 0; i < count; ++i) {
            atoms.addAtom(
                Vec3f(
                    (i % side) * 3.0,
                    ((i / side) % side) * 3.0,
                    (i / static_cast<double>(side * side)) * 3.0
                ),
                Vec3f::Random() * 0.5,
                AtomData::Type::H
            );
        }
        return atoms;
    }
};
