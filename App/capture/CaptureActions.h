#pragma once

#include "Signals/Signals.h"

class CaptureController;

namespace sf {
    class RenderWindow;
}

namespace CaptureActions {
    class Handler : public Signals::Trackable {
    public:
        Handler(CaptureController& captureController);
    };
}
