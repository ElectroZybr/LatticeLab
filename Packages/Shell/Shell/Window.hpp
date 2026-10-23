#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <vector>

#include <Actions/ActionRouter.hpp>

#include "FrameAPI.hpp"
#include "WindowAPI.hpp"

/// Window loop: pumps events, routes input and advances frames until every window
/// closes or a stop is requested. Composition runs it as the host service and assigns
/// the current windows, inputs and frames (between iterations, from idle()).
class WindowSystem final {
public:
    explicit WindowSystem(ActionRouter& router) : router_(router) {}

    // The shared router outlives this window system (it is mounted).
    ~WindowSystem() { setInputs({}); }

    void setWindows(std::vector<WindowAPI*> windows) { windows_ = std::move(windows); }
    void setFrames(std::vector<FrameAPI*> frames) { frames_ = std::move(frames); }

    /// Inputs feed the router: removed ones are unregistered, new ones registered.
    void setInputs(std::vector<InputAPI*> inputs) {
        for (InputAPI* input : inputs_)
            if (std::ranges::find(inputs, input) == inputs.end())
                router_.unregisterInput(*input);
        for (InputAPI* input : inputs)
            router_.registerInput(*input);
        inputs_ = std::move(inputs);
    }

    WindowSystem(const WindowSystem&) = delete;
    WindowSystem& operator=(const WindowSystem&) = delete;

    void run(const std::function<void()>& idle) {
        auto previous = std::chrono::steady_clock::now();

        while (!stop_) {
            idle();
            bool active = false;
            for (WindowAPI* window : windows_) {
                if (window->shouldClose())
                    continue;
                window->pollEvents();
                active = true;
            }
            if (!active)
                break;

            router_.tick();

            const auto now = std::chrono::steady_clock::now();
            const float dt = std::min(std::chrono::duration<float>(now - previous).count(), 0.1f);
            previous = now;

            for (FrameAPI* frame : frames_)
                frame->frame(dt);
        }
    }

    void requestStop() { stop_ = true; }

private:
    ActionRouter& router_;
    std::vector<WindowAPI*> windows_;
    std::vector<InputAPI*> inputs_;
    std::vector<FrameAPI*> frames_;
    std::atomic<bool> stop_{false};
};
