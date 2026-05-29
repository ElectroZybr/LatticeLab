#pragma once

#include <string_view>

#include <webgpu/webgpu-raii.hpp>

class Simulation;
class BaseRenderer;
struct PreviewFrameRect;
class CaptureController;

class AppStateIO {
public:
    static void save(CaptureController& captureController, const PreviewFrameRect& previewRect, const Simulation& simulation,
                     const BaseRenderer& renderer, std::string_view path);
    static void load(Simulation& simulation, BaseRenderer& renderer, std::string_view path);

private:
    static void saveText(CaptureController& captureController, const PreviewFrameRect& previewRect, const Simulation& simulation,
                         const BaseRenderer& renderer, std::string_view path);
    static void saveBinary(CaptureController& captureController, const PreviewFrameRect& previewRect, const Simulation& simulation,
                           const BaseRenderer& renderer, std::string_view path);

    static void loadText(Simulation& simulation, BaseRenderer& renderer, std::string_view path);
    static void loadBinary(Simulation& simulation, BaseRenderer& renderer, std::string_view path);
};
