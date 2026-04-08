#include "AppStateIO.h"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/io/SimulationStateIO.h"
#include "Rendering/BaseRenderer.h"

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

    sf::Image capturePreviewImage(const sf::RenderWindow& window) {
        sf::Texture texture;
        const sf::Vector2u windowSize = window.getSize();
        if (windowSize.x == 0 || windowSize.y == 0) {
            return {};
        }
        if (!texture.resize(windowSize)) {
            return {};
        }

        texture.update(window);
        const sf::Image fullImage = texture.copyToImage();

        const unsigned srcWidth = fullImage.getSize().x;
        const unsigned srcHeight = fullImage.getSize().y;
        if (srcWidth == 0 || srcHeight == 0) {
            return {};
        }

        const float scale = std::min(1.0f, std::min(static_cast<float>(kPreviewMaxSize) / static_cast<float>(srcWidth),
                                                    static_cast<float>(kPreviewMaxSize) / static_cast<float>(srcHeight)));
        const unsigned previewWidth = std::max(1u, static_cast<unsigned>(srcWidth * scale));
        const unsigned previewHeight = std::max(1u, static_cast<unsigned>(srcHeight * scale));

        sf::Image preview;
        preview.resize({previewWidth, previewHeight});
        for (unsigned y = 0; y < previewHeight; ++y) {
            for (unsigned x = 0; x < previewWidth; ++x) {
                const unsigned srcX = std::min(srcWidth - 1, static_cast<unsigned>((static_cast<std::uint64_t>(x) * srcWidth) / previewWidth));
                const unsigned srcY = std::min(srcHeight - 1, static_cast<unsigned>((static_cast<std::uint64_t>(y) * srcHeight) / previewHeight));
                preview.setPixel({x, y}, fullImage.getPixel({srcX, srcY}));
            }
        }

        return preview;
    }

    void saveImageState(const sf::RenderWindow& window, std::string_view path) {
        const sf::Image preview = capturePreviewImage(window);
        const sf::Vector2u size = preview.getSize();
        if (size.x == 0 || size.y == 0) {
            return;
        }

        const std::uint8_t* pixels = preview.getPixelsPtr();
        if (pixels == nullptr) {
            return;
        }

        const size_t byteCount = static_cast<size_t>(size.x) * static_cast<size_t>(size.y) * 4;
        const std::vector<std::uint8_t> bytes(pixels, pixels + byteCount);
        const std::string encoded = encodeBase64(bytes);

        std::ofstream file(path.data(), std::ios::app);
        if (!file.is_open()) {
            return;
        }

        file << "\n[image]\n";
        file << kBlockIndent << "encoding base64\n";
        file << kBlockIndent << "format rgba8\n";
        file << kBlockIndent << "width " << size.x << "\n";
        file << kBlockIndent << "height " << size.y << "\n";
        file << kBlockIndent << "data_begin\n";

        constexpr size_t lineWidth = 120;
        for (size_t offset = 0; offset < encoded.size(); offset += lineWidth) {
            file << kBlockIndent << encoded.substr(offset, lineWidth) << "\n";
        }

        file << kBlockIndent << "data_end\n";
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

void AppStateIO::save(const sf::RenderWindow& window, const Simulation& simulation, const IRenderer& renderer, std::string_view path) {
    SimulationStateIO::save(simulation, path);
    saveRendererState(renderer, path);
    saveImageState(window, path);
}

void AppStateIO::load(Simulation& simulation, IRenderer& renderer, std::string_view path) {
    SimulationStateIO::load(simulation, path);
    loadRendererState(renderer, path);
}
