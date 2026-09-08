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
        branch.use<WindowAPI, glfwWindow>();
        branch.add<Render>();
    }

    void configure(Lattice::Node& branch) {
        actionMap = branch.require<ActionMap>();
        render = branch.require<Render>();
        window = branch.find<WindowAPI>();

        branch.on("print", [&]() { print(); });

        window->show();
        window->setTitle("LatticeLab");
    }

    void run() override {
        // actionMap->set("verlet.dt");
        // actionMap->set("actions.print");
        // actionMap->set("io.load");
        // actionMap->bindAdd("dt", "MouseLeft", +0.001);
        actionMap->bind("print", "MouseLeft");
        actionMap->bind("load", "Ctrl+O");

        render->setup();
        while (!stopRequested()) {
            window->pollEvents();
            actionMap->tick();
            if (window->shouldClose()) {
                requestStop();
                break;
            }
            render->frame();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    ~Window() {
        stop();
        Logger::info("Window", "destroying object");
    }

private:
    Ref<Lattice::Settings> settings;
    Ref<ActionMap> actionMap;
    Ref<Render> render;
    Slot<WindowAPI> window;

    uint32_t fps;
    void print() {
        Logger::action("printer", "test");
    }
};