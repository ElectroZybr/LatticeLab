#pragma once

// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>
#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/Node.hpp>

// Plugin dependences
#include "ActionMap.hpp"

// Source
#include "Render.hpp"
#include "WindowAPI.hpp"
#include "glfwWindow/glfwWindow.hpp"


class Window final : public ServiceAPI {
public:
    explicit Window(Lattice::Node& branch) {
        branch.slot<WindowAPI>();
        branch.add<Render>();
        branch.add<ActionMap>();
    }

    void configure(Lattice::Node& branch) {
        actionMap = branch.require<ActionMap>();
        render = branch.require<Render>();
        window = branch.find<WindowAPI>();
        if (!window) window.use<glfwWindow>();

        branch.on("print", [this]() { print(); });
    }

    void run() override {
        while (!stopRequested()) {
            if (window) {
                window->pollEvents();
                if (window->shouldClose()) {
                    requestStop();
                    break;
                }
            }

            actionMap->tick();
            render->frame();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    ~Window() {
        stop();
        Logger::info("Window", "destroying object");
    }

private:
    Ref<ActionMap> actionMap;
    Ref<Render> render;
    Slot<WindowAPI> window;

    uint16_t fps = 0;
    void print() {
        Logger::action("printer", "test");
    }
};