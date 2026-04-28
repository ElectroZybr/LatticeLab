#pragma once

#ifdef _WIN32
#include <windows.h>
#ifdef NEAR
#undef NEAR
#endif
#ifdef FAR
#undef FAR
#endif
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
#endif

#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <iostream>

#include "generated/AppVersion.h"

inline GLFWwindow* createWindow() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int monitorX = 0;
    int monitorY = 0;
    glfwGetMonitorPos(monitor, &monitorX, &monitorY);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "LatticeLab " LATTICELAB_VERSION_STRING, nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwSetWindowPos(window, monitorX, monitorY);

#ifdef _WIN32
    if (HWND hwnd = glfwGetWin32Window(window)) {
        HINSTANCE instance = GetModuleHandleW(nullptr);
        if (HICON bigIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(101), IMAGE_ICON, 256, 256, LR_DEFAULTCOLOR))) {
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(bigIcon));
        }
        if (HICON smallIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(101), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR))) {
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
        }
    }
#endif

    return window;
}
