// Kernel dependences
#include <Lattice/Kernel/Plugin.hpp>

// Plugin dependences

// Sources
#include "glfwWindow/glfwWindow.hpp"
#include "InputAPI.hpp"
// #include "KeybindLoader.hpp"
// #include "LoaderAPI.hpp"
#include "WindowAPI.hpp"
#include "Window.hpp"
#include "Mouse.hpp"
#include "Keyboard.hpp"


extern "C" bool plugin_register(Lattice::Blueprints& reg) {
    reg.registerAPI<WindowAPI>();
    reg.registerImpl<glfwWindow, WindowAPI>();
    reg.registerImpl<Window, ServiceAPI>();

    reg.registerImpl<Input::Keyboard, InputAPI>();
    reg.registerImpl<Input::Mouse, InputAPI>();

    // reg.registerImpl<LoaderAPI, KeybindsLoader>();
    return true;
}

extern "C" void plugin_shutdown() {}
