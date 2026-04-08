#include "CaptureActions.h"

#include <filesystem>

#include "App/AppSignals.h"
#include "CaptureController.h"

namespace CaptureActions {
    Handler::Handler(sf::RenderWindow& window, CaptureController& captureController) {
        track(AppSignals::Capture::ToggleRecording.connect([&]() { captureController.toggle(window); }));
        track(AppSignals::Capture::SetOutputDirectory.connect([&](std::string_view path) {
            captureController.setOutputDirectory(std::filesystem::path(path));
        }));
    }
}
