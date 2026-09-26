#pragma once

#include <algorithm>
#include <chrono>

#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Kernel/ServiceAPI.hpp>

#include "ActionRouter.hpp"
#include "Render.hpp"


class WindowSystem final : public ServiceAPI {
public:
    explicit WindowSystem(NodeBuild branch) {
        router_ = branch.add<ActionRouter>();
        branch.add<Render>();
    }

    void configure(NodeConfigure branch) {
        windows_ = branch.children<WindowAPI>();
        renders_ = branch.children<Render>();
        for (InputAPI* input : branch.collect<InputAPI>())
            router_->registerInput(*input);
    }

    void run() override {
        if (windows_.empty())
            return;

        auto previous = std::chrono::steady_clock::now();

        while (!stopRequested()) {
            bool active = false;
            for (WindowAPI* window : windows_) {
                if (window->shouldClose())
                    continue;
                window->pollEvents();
                active = true;
            }

            if (!active) {
                requestStop();
                break;
            }

            router_->tick();

            const auto now = std::chrono::steady_clock::now();
            const float dt = std::min(
                std::chrono::duration<float>(now - previous).count(),
                0.1f
            );
            previous = now;

            for (Render* render : renders_)
                render->frame(dt);
        }
    }

private:
    Ref<ActionRouter> router_;
    Children<WindowAPI> windows_;
    Children<Render> renders_;
};
