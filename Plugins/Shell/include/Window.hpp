#pragma once

// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/NodeViews.hpp>
#include "Lattice/Kernel/NodeHandlers.hpp"

// Plugin dependences
#include "ActionRouter.hpp"

// Source
#include "Render.hpp"
#include "WindowAPI.hpp"
#include <chrono>


class WindowSystem final : public ServiceAPI {
    Ref<ActionRouter> router_;
    Children<Render> render_;

public:
    explicit WindowSystem(NodeBuild branch) {
        router_ = branch.add<ActionRouter>();
        branch.add<Render>();
    }

    void configure(NodeConfigure branch) {
        render_ = branch.children<Render>();
    }

    void run() override {
        auto previous = std::chrono::steady_clock::now();

        while (!stopRequested()) {
            // for (auto* window : windows_)
            //     window->pollEvents();

            router_->tick();

            const auto now = std::chrono::steady_clock::now();
            const float dt = std::min(std::chrono::duration<float>(now - previous).count(), 0.1f);
            previous = now;

            for (auto* render : render_)
                render->frame(dt);
        }
    }
};
