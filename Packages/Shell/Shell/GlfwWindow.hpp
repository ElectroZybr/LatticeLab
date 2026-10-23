#pragma once

#include <GLFW/glfw3.h>
#include <string_view>

#include "WindowAPI.hpp"

class GlfwKeyboard;
class GlfwMouse;

class GlfwWindow final : public WindowAPI {
public:
    GlfwWindow(GlfwKeyboard& keyboard, GlfwMouse& mouse);

    ~GlfwWindow() override;

    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;

    GlfwWindow(GlfwWindow&&) = delete;
    GlfwWindow& operator=(GlfwWindow&&) = delete;

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

    GlfwKeyboard* keyboard_;
    GlfwMouse* mouse_;
    std::shared_ptr<GLFWwindow> windowOwner_;
    GLFWwindow* window_ = nullptr;
    State state_{};
};
