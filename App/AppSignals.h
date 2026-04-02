#include <string_view>

#include "Signals/Signals.h"
#include "Engine/math/Vec3f.h"

#include "GUI/interface/panels/tools/ToolsPanel.h"
#include "Rendering/camera/Camera.h"

namespace AppSignals {
    inline Signals::Signal<void(const Vec3f& oldSize, const Vec3f& newSize)> ResizeBox;

    namespace UI {
        inline Signals::Signal<void(const Vec3f& newSize)> ResizeBox;

        inline Signals::Signal<void()> ClearSimulation;
        inline Signals::Signal<void()> CreateGas;
        inline Signals::Signal<void()> CreateCrystal;

        inline Signals::Signal<void(RendererType type)> SetRender;
        inline Signals::Signal<void(Camera::Mode mode)> SetCameraMode;

        inline Signals::Signal<void(std::string_view path)> SaveSimulation;
        inline Signals::Signal<void(std::string_view path)> LoadSimulation;

        inline Signals::Signal<void()> ExitApplication;

        inline Signals::Signal<void()> StepPhysics;
    }

    namespace Keyboard {
        inline Signals::Signal<void()> StepPhysics;
    }
}
