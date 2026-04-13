#include <SFML/Graphics/RenderWindow.hpp>
#include <bgfx/bgfx.h>

#if defined(SFML_SYSTEM_LINUX)
#include <GL/glx.h>
#include <X11/Xlib.h>
#elif defined(SFML_SYSTEM_WINDOWS)
// clang-format off
#include <windows.h>
#include <GL/gl.h>
// clang-format on
#elif defined(SFML_SYSTEM_MACOS)
#include <OpenGL/OpenGL.h>
#endif

#include "Rendering/BgfxCallback.h"

class BgfxContext {
public:
    static BgfxContext& instance() {
        static BgfxContext ctx;
        return ctx;
    }

    void init(sf::WindowHandle nativeHandle, uint32_t width, uint32_t height) {
        if (initialized) {
            return;
        }

        bgfx::renderFrame();

        bgfx::Init init;
        init.type = bgfx::RendererType::OpenGL;
#if defined(SFML_SYSTEM_LINUX)
        init.platformData.ndt = XOpenDisplay(nullptr);
        init.platformData.context = reinterpret_cast<void*>(glXGetCurrentContext());
#elif defined(SFML_SYSTEM_WINDOWS)
        init.platformData.context = reinterpret_cast<void*>(wglGetCurrentContext());
#elif defined(SFML_SYSTEM_MACOS)
        init.platformData.context = reinterpret_cast<void*>(CGLGetCurrentContext());
#endif
        init.platformData.nwh = reinterpret_cast<void*>(nativeHandle);
        init.resolution.width = width;
        init.resolution.height = height;
        init.resolution.reset = BGFX_RESET_NONE;

        init.callback = &callback_;

        if (!bgfx::init(init)) {
            throw std::runtime_error("bgfx::init failed");
        }
        initialized = true;
    }

    void init(uint32_t width, uint32_t height) {
        if (initialized) {
            return;
        }

        bgfx::renderFrame();

        bgfx::Init init;
        init.type = bgfx::RendererType::OpenGL;
#if defined(SFML_SYSTEM_LINUX)
        init.platformData.ndt = XOpenDisplay(nullptr);
        init.platformData.context = reinterpret_cast<void*>(glXGetCurrentContext());
#elif defined(SFML_SYSTEM_WINDOWS)
        init.platformData.context = reinterpret_cast<void*>(wglGetCurrentContext());
#elif defined(SFML_SYSTEM_MACOS)
        init.platformData.context = reinterpret_cast<void*>(CGLGetCurrentContext());
#endif
        init.resolution.width = width;
        init.resolution.height = height;
        init.resolution.reset = BGFX_RESET_NONE;

        init.callback = &callback_;

        if (!bgfx::init(init)) {
            throw std::runtime_error("bgfx::init failed");
        }
        initialized = true;
    }

    void shutdown() {
        if (!initialized) {
            return;
        }
        bgfx::shutdown();
        initialized = false;
    }

    ~BgfxContext() { shutdown(); }

    BgfxCallback& callback() { return callback_; }

private:
    BgfxContext() = default;
    bool initialized = false;
    BgfxCallback callback_;
};
