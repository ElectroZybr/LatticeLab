// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>

// Plugin dependences

// Sources
#include "glfwWindow/glfwWindow.hpp"
#include "InputAPI.hpp"
#include "KeybindLoader.hpp"
#include "LoaderAPI.hpp"
#include "WindowAPI.hpp"
#include "Window.hpp"
#include "Mouse.hpp"
#include "Keyboard.hpp"


extern "C" bool plugin_register(Lattice::Node& blueprints) {
    blueprints.blueprint<WindowAPI>();
    blueprints.blueprint<glfwWindow, WindowAPI>();
    blueprints.blueprint<Window, ServiceAPI>();

    blueprints.blueprint<Input::Keyboard, InputAPI>();
    blueprints.blueprint<Input::Mouse, InputAPI>();

    blueprints.blueprint<KeybindsLoader, LoaderAPI>();
    return true;
}

extern "C" void plugin_shutdown() {}
