#pragma once

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>
#include <cmath>

#include "Engine/physics/Atom.h"
#include "Engine/SimBox.h"
#include "Rendering/2d/Renderer2D.h"

class RendererFixture : public benchmark::Fixture {
public:
    void SetUp(benchmark::State& state) override {
        renderTexture_ = std::make_unique<sf::RenderTexture>();
        if (!renderTexture_->resize({800, 600})) {
            state.SkipWithError("RenderTexture resize failed");
            return;
        }

        view_ = renderTexture_->getView();;
        renderer_ = std::make_unique<Renderer2D>(*renderTexture_, view_);

        atoms_ = makeGridAtoms(static_cast<int>(state.range(0)));
    }

    void TearDown(benchmark::State&) override {
        renderer_.reset();
    }

protected:
    void setCounters(benchmark::State& state) const {
        state.SetItemsProcessed(
            state.iterations() * static_cast<int64_t>(atoms_.size())
        );
    }

    std::unique_ptr<sf::RenderTexture> renderTexture_;
    std::unique_ptr<Renderer2D> renderer_;
    sf::View view_;
    std::vector<Atom> atoms_;
    SimBox box_{ Vec3D(0, 0, 0), Vec3D(300, 300, 300) };

private:
    static std::vector<Atom> makeGridAtoms(int count) {
        std::vector<Atom> atoms;
        atoms.reserve(count);
        const int side = static_cast<int>(std::cbrt(count)) + 1;
        for (int i = 0; i < count; ++i) {
            atoms.emplace_back(
                Vec3D(
                    (i % side) * 3.0,
                    ((i / side) % side) * 3.0,
                    (i / static_cast<double>(side * side)) * 3.0
                ),
                Vec3D::Random() * 0.5,
                Atom::Type::H
            );
        }
        return atoms;
    }
};