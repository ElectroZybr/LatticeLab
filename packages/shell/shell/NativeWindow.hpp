#pragma once

#include <memory>

/// Native window handles a presentation surface is created from. Produced by the
/// window layer, consumed by the renderer; no GPU types here.
struct NativeWindow {
    enum class Kind { None, Win32, X11, Wayland, Cocoa, Metal, Web, Headless };
    Kind kind = Kind::None;
    void* display = nullptr;
    void* window = nullptr;
    void* extra = nullptr;
    std::shared_ptr<void> owner;
    bool operator==(const NativeWindow&) const = default;
};
