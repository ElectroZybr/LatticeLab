#pragma once

#include <GLFW/glfw3.h>
#include <string_view>

#include <Lattice/Kernel/NodeViews.hpp>
#include "WindowAPI.hpp"

class glfwKeyboard;
class glfwMouse;

class glfwWindow final : public WindowAPI {
public:
    explicit glfwWindow(NodeBuild);
    void configure(NodeConfigure);

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

    NativeWindow native() const override;
    void show() override;
    void setTitle(std::string_view title) override;

private:
    static constexpr std::string_view tag = "glfwWindow";
    
    void setupCallbacks();
    void syncFromWindow();
    GLFWmonitor* currentMonitor() const;
    GLFWmonitor* monitorByIndex(int index) const;
    int monitorIndex(GLFWmonitor* monitor) const;
    void applyWindowed();
    void applyFullscreen(GLFWmonitor* monitor);

    Ref<glfwKeyboard> keyboard_;
    Ref<glfwMouse> mouse_;
    std::shared_ptr<GLFWwindow> windowOwner_;
    GLFWwindow* window_ = nullptr;
    State state_{};
};