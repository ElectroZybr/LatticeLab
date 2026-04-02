#include "Signals/Signals.h"

#include "Engine/math/Vec3f.h"

namespace AppSignals {
    inline Signals::Signal<void(const Vec3f& oldSize, const Vec3f& newSize)> ResizeBox;

    namespace UI {
        inline Signals::Signal<void(const Vec3f& newSize)> ResizeBox;

        inline Signals::Signal<void()> ClearSimulation;
        inline Signals::Signal<void()> CreateGas;
        inline Signals::Signal<void()> CreateCrystal;
    }
}
