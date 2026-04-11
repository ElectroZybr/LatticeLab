#include <SFML/Graphics/RenderWindow.hpp>
#include <bgfx/bgfx.h>

#if defined(SFML_SYSTEM_LINUX)
#include <GL/glx.h>
#include <X11/Xlib.h>
#endif

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
        init.platformData.nwh = reinterpret_cast<void*>(nativeHandle);
        init.platformData.context = reinterpret_cast<void*>(glXGetCurrentContext());
#elif defined(SFML_SYSTEM_WINDOWS)
        init.platformData.nwh = reinterpret_cast<void*>(nativeHandle);
#elif defined(SFML_SYSTEM_MACOS)
        init.platformData.nwh = reinterpret_cast<void*>(nativeHandle);
#endif
        init.resolution.width = width;
        init.resolution.height = height;
        init.resolution.reset = BGFX_RESET_NONE;

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

private:
    BgfxContext() = default;
    bool initialized = false;
};
