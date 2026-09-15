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
    }

    void configure(Lattice::Node& branch) {
        actionMap = branch.require<ActionMap>();
        render = branch.require<Render>();
        window = branch.find<WindowAPI>();
        window.use("glfwWindow");

        branch.on("print", [this]() { print(); });

        // window->show();
        // window->setTitle("LatticeLab");
    }

    void run() override {
        // actionMap->set("verlet.dt");
        // actionMap->set("actions.print");
        // actionMap->set("io.load");
        // actionMap->bindAdd("dt", "MouseLeft", +0.001);
        actionMap->bind("print", "MouseLeft");
        actionMap->bind("load", "Ctrl+O");
        // actionMap->run_ctx->activate(0, 48);
        // actionMap->run_ctx->activate(1, 49);

        render->setup();
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
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
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

    uint32_t fps;
    void print() {
        Logger::action("printer", "test");
    }
};