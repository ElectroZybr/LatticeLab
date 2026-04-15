#include <stdexcept>

#if defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#elif defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "Rendering/BgfxCallback.h"

class BgfxContext {
public:
    static BgfxContext& instance() {
        static BgfxContext ctx;
        return ctx;
    }

    void init(GLFWwindow* window, uint32_t width, uint32_t height) {
        if (initialized) {
            return;
        }

        bgfx::PlatformData pd;
#if defined(__linux__)
        pd.ndt = glfwGetX11Display();
#else
        pd.ndt = nullptr;
#endif
        pd.nwh = nullptr;
        pd.context = nullptr;
        pd.backBuffer = nullptr;
        pd.backBufferDS = nullptr;

        if (window != nullptr) {
#if defined(__linux__)
            pd.nwh = reinterpret_cast<void*>(static_cast<uintptr_t>(glfwGetX11Window(window)));
#elif defined(_WIN32)
            pd.nwh = glfwGetWin32Window(window);
#elif defined(__APPLE__)
            pd.nwh = glfwGetCocoaWindow(window);
#endif
        }

        bgfx::renderFrame();

        bgfx::Init init;
        init.type = bgfx::RendererType::Count;
        init.vendorId = BGFX_PCI_ID_NONE;
        init.platformData = pd;

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
