#pragma once

#include <string_view>

#include "Engine/physics/AtomData.h"
#include "Engine/math/Vec3f.h"
#include "GUI/interface/panels/tools/ToolsPanel.h"
#include "Rendering/camera/Camera.h"
#include "Signals/Signals.h"

namespace AppSignals {
    namespace UI {
        inline Signals::Signal<void(const Vec3f& newSize)> ResizeBox;

        inline Signals::Signal<void()> ClearSimulation;
        inline Signals::Signal<void(int atomCount, AtomData::Type atomType, bool is3D, float density)> CreateGas;
        inline Signals::Signal<void(int axisCount, AtomData::Type atomType, bool is3D)> CreateCrystal;

        inline Signals::Signal<void(RendererType type)> SetRender;
        inline Signals::Signal<void(Camera::Mode mode)> SetCameraMode;

        inline Signals::Signal<void(std::string_view path)> SaveSimulation;
        inline Signals::Signal<void(std::string_view path)> LoadSimulation;
        inline Signals::Signal<void(std::string_view path)> SetCaptureDirectory;
        inline Signals::Signal<void()> ToggleCapture;

        inline Signals::Signal<void()> ExitApplication;

        inline Signals::Signal<void()> StepPhysics;
    }

    namespace Keyboard {
        inline Signals::Signal<void()> StepPhysics;
    }
}
