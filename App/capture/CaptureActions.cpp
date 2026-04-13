#include "CaptureActions.h"

#include "CaptureController.h"

#include <filesystem>

#include "App/AppSignals.h"

namespace CaptureActions {
    Handler::Handler(CaptureController& captureController) {
        track(AppSignals::Capture::ToggleRecording.connect([&]() { captureController.toggle(); }));
        track(AppSignals::Capture::SetOutputDirectory.connect(
            [&](std::string_view path) { captureController.setOutputDirectory(std::filesystem::path(path)); }));
    }
}
