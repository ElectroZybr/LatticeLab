#pragma once

// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/NodeViews.hpp>

// Plugin dependences
#include "ActionRouter.hpp"

// Source
#include "Render.hpp"
#include "WindowAPI.hpp"
#include "glfwWindow/glfwWindow.hpp"
#include <chrono>


class Window final : public ServiceAPI {
public:
    explicit Window(NodeBuild branch) {
        window = branch.addSlot<WindowAPI>();
        render = branch.add<Render>();
        router = branch.add<ActionRouter>();
    }

    void configure(NodeConfigure branch) {
        if (!window.exists()) window.choice<glfwWindow>();
    }
    
    void run() override {
        router->bindAxis2("look", "MouseDelta+MouseLeft");
        router->bindAxis2("orbit", "MouseDelta+MouseLeft");
        router->bindAxis2("pan", "MouseDelta+Ctrl+MouseLeft");
        router->bindAxis2("zoom", "MouseWheel");
        router->bindAxis2("cursor", "MousePos");

        auto previous = std::chrono::steady_clock::now();
        while (!stopRequested()) {
            if (window) {
                window->pollEvents();
                if (window->shouldClose()) {
                    requestStop();
                    break;
                }
            }

            router->tick();
            const auto now = std::chrono::steady_clock::now();
            const float dt = std::min(std::chrono::duration<float>(now - previous).count(), 0.1f);
            previous = now;
            render->frame(dt);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    ~Window() {
        stop();
        Logger::info("Window", "destroying object");
    }

private:
    Slot<WindowAPI> window;
    Ref<ActionRouter> router;
    Ref<Render> render;

    uint16_t fps = 0;
};
