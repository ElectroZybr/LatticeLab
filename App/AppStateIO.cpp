#include "AppStateIO.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <bgfx/bgfx.h>

#include "Engine/io/SimulationStateIO.h"
#include "GUI/interface/UiState.h"
#include "Rendering/BaseRenderer.h"
#include "Rendering/BgfxContext.h"

namespace {
    constexpr const char* kBlockIndent = "  ";
    constexpr unsigned kPreviewMaxSize = 256;

    struct LoadedRendererData {
        bool drawGrid = false;
        bool drawBonds = false;
        int speedColorMode = static_cast<int>(IRenderer::SpeedColorMode::AtomColor);
        float speedGradientMax = 5.0f;
        float alpha = 0.05f;
    };

    std::string trim(std::string_view value) {
        size_t begin = 0;
        while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
            ++begin;
        }

        size_t end = value.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
            --end;
        }

        return std::string(value.substr(begin, end - begin));
    }

    std::string encodeBase64(const std::vector<std::uint8_t>& data) {
        static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string encoded;
        encoded.reserve(((data.size() + 2) / 3) * 4);

        for (size_t i = 0; i < data.size(); i += 3) {
            const std::uint32_t a = data[i];
            const std::uint32_t b = (i + 1 < data.size()) ? data[i + 1] : 0;
            const std::uint32_t c = (i + 2 < data.size()) ? data[i + 2] : 0;
            const std::uint32_t triple = (a << 16) | (b << 8) | c;

            encoded.push_back(kAlphabet[(triple >> 18) & 0x3F]);
            encoded.push_back(kAlphabet[(triple >> 12) & 0x3F]);
            encoded.push_back((i + 1 < data.size()) ? kAlphabet[(triple >> 6) & 0x3F] : '=');
            encoded.push_back((i + 2 < data.size()) ? kAlphabet[triple & 0x3F] : '=');
        }

        return encoded;
    }

    ImageData capturePreviewImage(uint32_t srcWidth, uint32_t srcHeight, const void* rawData, uint32_t pitch, bool yflip,
                                  const PreviewFrameRect& previewRect) {
        if (srcWidth == 0 || srcHeight == 0 || rawData == nullptr) {
            return {};
        }

        const std::uint8_t* src = static_cast<const std::uint8_t*>(rawData);

        const unsigned cropX =
            std::min(srcWidth - 1, static_cast<unsigned>(std::clamp(previewRect.x, 0.0f, static_cast<float>(srcWidth - 1))));
        const unsigned cropY =
            std::min(srcHeight - 1, static_cast<unsigned>(std::clamp(previewRect.y, 0.0f, static_cast<float>(srcHeight - 1))));
        const unsigned cropWidth = std::max(
            1u, std::min(srcWidth - cropX, static_cast<unsigned>(std::clamp(previewRect.width, 1.0f, static_cast<float>(srcWidth)))));
        const unsigned cropHeight = std::max(
            1u, std::min(srcHeight - cropY, static_cast<unsigned>(std::clamp(previewRect.height, 1.0f, static_cast<float>(srcHeight)))));

        const float scale = std::min(1.0f, std::min(static_cast<float>(kPreviewMaxSize) / static_cast<float>(cropWidth),
                                                    static_cast<float>(kPreviewMaxSize) / static_cast<float>(cropHeight)));
        const unsigned previewWidth = std::max(1u, static_cast<unsigned>(cropWidth * scale));
        const unsigned previewHeight = std::max(1u, static_cast<unsigned>(cropHeight * scale));

        ImageData result;
        result.width = previewWidth;
        result.height = previewHeight;
        result.pixels.resize(static_cast<size_t>(previewWidth) * previewHeight * 4);

        for (unsigned y = 0; y < previewHeight; ++y) {
            for (unsigned x = 0; x < previewWidth; ++x) {
                unsigned srcSampleX =
                    cropX + std::min(cropWidth - 1, static_cast<unsigned>((static_cast<std::uint64_t>(x) * cropWidth) / previewWidth));
                unsigned srcSampleY =
                    cropY + std::min(cropHeight - 1, static_cast<unsigned>((static_cast<std::uint64_t>(y) * cropHeight) / previewHeight));

                unsigned flippedY = yflip ? (srcHeight - 1 - srcSampleY) : srcSampleY;

                const std::uint8_t* srcPixel = src + flippedY * pitch + srcSampleX * 4;
                std::uint8_t* dstPixel = result.pixels.data() + (y * previewWidth + x) * 4;

                dstPixel[0] = srcPixel[0];
                dstPixel[1] = srcPixel[1];
                dstPixel[2] = srcPixel[2];
                dstPixel[3] = srcPixel[3];
            }
        }
        return result;
    }

    void saveImageState(const PreviewFrameRect& previewRect, std::string_view path) {
        BgfxCallback& bgfxCallback = BgfxContext::instance().callback();

        bgfxCallback.addScreenShotCallback(path, [&bgfxCallback, path, previewRect, filePath = std::string(path)](
                                                     uint32_t width, uint32_t height, const void* data, uint32_t size, bool yflip) {
            bgfxCallback.removeScreenShotCallback(path);

            const uint32_t pitch = width * 4;
            const ImageData preview = capturePreviewImage(width, height, data, pitch, yflip, previewRect);

            if (preview.width == 0 || preview.height == 0) {
                return;
            }

            const size_t byteCount = static_cast<size_t>(preview.width) * preview.height * 4;
            const std::vector<std::uint8_t> bytes(preview.pixels.data(), preview.pixels.data() + byteCount);
            const std::string encoded = encodeBase64(bytes);

            std::ofstream file(filePath, std::ios::app);
            if (!file.is_open()) {
                return;
            }

            file << "\n[image]\n";
            file << kBlockIndent << "encoding base64\n";
            file << kBlockIndent << "format rgba8\n";
            file << kBlockIndent << "width " << preview.width << "\n";
            file << kBlockIndent << "height " << preview.height << "\n";
            file << kBlockIndent << "data_begin\n";

            constexpr size_t lineWidth = 120;
            for (size_t offset = 0; offset < encoded.size(); offset += lineWidth) {
                file << kBlockIndent << encoded.substr(offset, lineWidth) << "\n";
            }

            file << kBlockIndent << "data_end\n";
        });

        bgfx::requestScreenShot(BGFX_INVALID_HANDLE, path.data());
    }

    void saveRendererState(const IRenderer& renderer, std::string_view path) {
        std::ofstream file(path.data(), std::ios::app);
        if (!file.is_open()) {
            return;
        }

        file << "\n[renderer]\n";
        file << kBlockIndent << "draw_grid " << static_cast<int>(renderer.drawGrid) << "\n";
        file << kBlockIndent << "draw_bonds " << static_cast<int>(renderer.drawBonds) << "\n";
        file << kBlockIndent << "speed_color_mode " << static_cast<int>(renderer.speedColorMode) << "\n";
        file << kBlockIndent << "speed_gradient_max " << renderer.speedGradientMax << "\n";
        file << kBlockIndent << "renderer_alpha " << renderer.alpha << "\n";
    }

    void loadRendererState(IRenderer& renderer, std::string_view path) {
        std::ifstream file(path.data());
        if (!file.is_open()) {
            return;
        }

        LoadedRendererData loadedRenderer{
            .drawGrid = renderer.drawGrid,
            .drawBonds = renderer.drawBonds,
            .speedColorMode = static_cast<int>(renderer.speedColorMode),
            .speedGradientMax = renderer.speedGradientMax,
            .alpha = renderer.alpha,
        };

        std::string line;
        while (std::getline(file, line)) {
            const std::string trimmed = trim(line);
            if (trimmed.empty() || trimmed.front() == '#' || trimmed.front() == '[') {
                continue;
            }

            std::istringstream stream(trimmed);
            std::string tag;
            stream >> tag;

            if (tag == "draw_grid") {
                int value = 0;
                stream >> value;
                loadedRenderer.drawGrid = (value != 0);
            }
            else if (tag == "draw_bonds") {
                int value = 0;
                stream >> value;
                loadedRenderer.drawBonds = (value != 0);
            }
            else if (tag == "speed_color_mode") {
                stream >> loadedRenderer.speedColorMode;
            }
            else if (tag == "speed_gradient_max") {
                stream >> loadedRenderer.speedGradientMax;
            }
            else if (tag == "renderer_alpha") {
                stream >> loadedRenderer.alpha;
            }
        }

        renderer.drawGrid = loadedRenderer.drawGrid;
        renderer.drawBonds = loadedRenderer.drawBonds;
        renderer.speedColorMode = static_cast<IRenderer::SpeedColorMode>(loadedRenderer.speedColorMode);
        renderer.speedGradientMax = loadedRenderer.speedGradientMax;
        renderer.alpha = loadedRenderer.alpha;
    }
}

void AppStateIO::save(const sf::RenderWindow& window, const PreviewFrameRect& previewRect, const Simulation& simulation,
                      const IRenderer& renderer, std::string_view path) {
    SimulationStateIO::save(simulation, path);
    saveRendererState(renderer, path);
    saveImageState(previewRect, path);
}

void AppStateIO::load(Simulation& simulation, IRenderer& renderer, std::string_view path) {
    SimulationStateIO::load(simulation, path);
    loadRendererState(renderer, path);
}
