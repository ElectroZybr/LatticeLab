#pragma once

#include <GLFW/glfw3.h>
#include <string_view>

#include "WindowAPI.hpp"

class glfwKeyboard;
class glfwMouse;

class glfwWindow final : public WindowAPI {
public:
    glfwWindow(glfwKeyboard& keyboard, glfwMouse& mouse);

    ~glfwWindow() override;

    glfwWindow(const glfwWindow&) = delete;
    glfwWindow& operator=(const glfwWindow&) = delete;

    glfwWindow(glfwWindow&&) = delete;
    glfwWindow& operator=(glfwWindow&&) = delete;

    // WindowAPI
    bool shouldClose() const override;
    void requestClose() override;
    void pollEvents() override;

    glm::vec2 windowSize() const override;
    glm::vec2 framebufferSize() const override;
    float contentScale() const override;
    bool fullscreen() const override;
    void setFullscreen(bool enabled) override;

    WGPU::NativeWindow native() const override;
    void show() override;
    void setTitle(std::string_view title) override;

private:
    void createWindow();
    void setupCallbacks();
    void syncFromWindow();
    GLFWmonitor* currentMonitor() const;
    GLFWmonitor* monitorByIndex(int index) const;
    int monitorIndex(GLFWmonitor* monitor) const;
    void applyWindowed();
    void applyFullscreen(GLFWmonitor* monitor);

    glfwKeyboard* keyboard_;
    glfwMouse* mouse_;
    std::shared_ptr<GLFWwindow> windowOwner_;
    GLFWwindow* window_ = nullptr;
    State state_{};
};
