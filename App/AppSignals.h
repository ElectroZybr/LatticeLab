#include "Signals/Signals.h"

#include "Engine/math/Vec3f.h"

namespace AppSignals {
    inline Signals::Signal<void(const Vec3f& oldSize, const Vec3f& newSize)> ResizeBox;

    namespace UI {
        inline Signals::Signal<void(const Vec3f& newSize)> ResizeBox;
    }
}
