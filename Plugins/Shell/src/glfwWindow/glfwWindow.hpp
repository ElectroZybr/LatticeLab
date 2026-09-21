#pragma once

#include <GLFW/glfw3.h>
#include <string_view>

#include <Lattice/Kernel/Node.hpp>
#include "WindowAPI.hpp"
#include "glfwWindow/glfwKeyboard.hpp"
#include "glfwWindow/glfwMouse.hpp"

class glfwWindow final : public WindowAPI {
public:
    explicit glfwWindow(Lattice::Node& branch);
    void configure(Lattice::Node& branch);

    ~glfwWindow() override;

    glfwWindow(const glfwWindow&) = delete;
    glfwWindow& operator=(const glfwWindow&) = delete;
    glfwWindow(glfwWindow&&) noexcept;
    glfwWindow& operator=(glfwWindow&&) noexcept;

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