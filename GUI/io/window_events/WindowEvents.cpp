#include "WindowEvents.h"

#include "imgui_impl_bgfx.h"

#include "GUI/interface/interface.h"
#include "Rendering/BaseRenderer.h"

GLFWwindow* WindowEvents::window = nullptr;
std::unique_ptr<IRenderer>* WindowEvents::renderer = nullptr;
Interface* WindowEvents::appInterface = nullptr;

void WindowEvents::init(GLFWwindow* w, std::unique_ptr<IRenderer>& r, Interface& appInterface) {
    window = w;
    renderer = &r;
    WindowEvents::appInterface = &appInterface;

    glfwSetWindowCloseCallback(window, windowCloseCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
}

void WindowEvents::windowCloseCallback(GLFWwindow* window) { glfwSetWindowShouldClose(window, GLFW_TRUE); }

void WindowEvents::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    (*renderer)->camera.setScreenSize(Vec2f(width, height));
    bgfx::reset(static_cast<uint32_t>(width), static_cast<uint32_t>(height), BGFX_RESET_NONE);

    if (appInterface == nullptr) {
        return;
    }

    appInterface->styleManager.onResize(Vec2i(width, height));

    if (appInterface->fontManager.load(appInterface->styleManager.getScale())) {
        ImGui_Implbgfx_InvalidateDeviceObjects();
        ImGui_Implbgfx_CreateDeviceObjects();
    }
}
