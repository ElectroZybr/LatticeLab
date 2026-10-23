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
/// closes or a stop is requested. Composition runs it as the host service.
class WindowSystem final {
public:
    WindowSystem(ActionRouter& router, std::vector<WindowAPI*> windows,
                 std::vector<InputAPI*> inputs, std::vector<FrameAPI*> frames)
        : router_(router), windows_(std::move(windows)), inputs_(std::move(inputs)), frames_(std::move(frames)) {
        for (InputAPI* input : inputs_)
            router_.registerInput(*input);
    }

    // The shared router outlives this window system (it is mounted).
    ~WindowSystem() {
        for (InputAPI* input : inputs_)
            router_.unregisterInput(*input);
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
