#include "WindowEvents.h"

#include <backends/imgui_impl_wgpu.h>

#include "Engine/math/Vec2.h"
#include "GUI/interface/interface.h"
#include "Rendering/BaseRenderer.h"
#include "Rendering/WGPUContext.h"

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
    WGPUContext::instance().resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    if (appInterface == nullptr) {
        return;
    }

    appInterface->styleManager.onResize(Vec2i(width, height));

    if (appInterface->fontManager.load(appInterface->styleManager.getScale())) {
        ImGui_ImplWGPU_InvalidateDeviceObjects();
        ImGui_ImplWGPU_CreateDeviceObjects();
    }
}
