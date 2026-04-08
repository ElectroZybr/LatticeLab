#include "ioPanelSceneCatalog.h"

#include <SFML/Graphics/Image.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace {
    struct ParsedSceneInfo {
        std::string title;
        std::string description;
        unsigned imageWidth = 0;
        unsigned imageHeight = 0;
        bool hasEmbeddedPreview = false;
        std::vector<std::uint8_t> imageBytes;
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

    std::string valueAfterTag(std::string_view line, std::string_view tag) {
        if (!line.starts_with(tag)) {
            return {};
        }
        return trim(line.substr(tag.size()));
    }

    std::vector<std::uint8_t> decodeBase64(std::string_view encoded) {
        std::array<int, 256> decodeTable{};
        decodeTable.fill(-1);

        constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < 64; ++i) {
            decodeTable[static_cast<unsigned char>(alphabet[i])] = i;
        }

        std::vector<std::uint8_t> decoded;
        decoded.reserve((encoded.size() / 4) * 3);

        int val = 0;
        int valb = -8;
        for (unsigned char c : encoded) {
            if (std::isspace(c)) {
                continue;
            }
            if (c == '=') {
                break;
            }
            const int decodedChar = decodeTable[c];
            if (decodedChar < 0) {
                continue;
            }
            val = (val << 6) + decodedChar;
            valb += 6;
            if (valb >= 0) {
                decoded.push_back(static_cast<std::uint8_t>((val >> valb) & 0xFF));
                valb -= 8;
            }
        }

        return decoded;
    }

    ParsedSceneInfo parseSceneInfo(const std::filesystem::path& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return {};
        }

        ParsedSceneInfo info;
        std::string section;
        std::string imageEncoding;
        std::string imageFormat;
        std::string imageDataBase64;
        bool readingImageData = false;

        std::string line;
        while (std::getline(file, line)) {
            const std::string trimmed = trim(line);
            if (trimmed.empty() || trimmed.starts_with('#')) {
                continue;
            }

            if (readingImageData) {
                if (trimmed == "data_end") {
                    readingImageData = false;
                }
                else {
                    imageDataBase64 += trimmed;
                }
                continue;
            }

            if (trimmed.front() == '[') {
                section = trimmed;
                continue;
            }

            if (section == "[meta]") {
                if (trimmed.starts_with("title ")) {
                    info.title = valueAfterTag(trimmed, "title");
                }
                else if (trimmed.starts_with("description ")) {
                    info.description = valueAfterTag(trimmed, "description");
                }
            }
            else if (section == "[image]") {
                if (trimmed.starts_with("encoding ")) {
                    imageEncoding = valueAfterTag(trimmed, "encoding");
                }
                else if (trimmed.starts_with("format ")) {
                    imageFormat = valueAfterTag(trimmed, "format");
                }
                else if (trimmed.starts_with("width ")) {
                    info.imageWidth = static_cast<unsigned>(std::max(0, std::stoi(valueAfterTag(trimmed, "width"))));
                }
                else if (trimmed.starts_with("height ")) {
                    info.imageHeight = static_cast<unsigned>(std::max(0, std::stoi(valueAfterTag(trimmed, "height"))));
                }
                else if (trimmed == "data_begin") {
                    readingImageData = true;
                }
            }
        }

        if (info.title.empty()) {
            info.title = path.stem().string();
        }

        if (imageEncoding == "base64" && imageFormat == "rgba8" && info.imageWidth > 0 && info.imageHeight > 0) {
            info.imageBytes = decodeBase64(imageDataBase64);
            const size_t expectedSize = static_cast<size_t>(info.imageWidth) * static_cast<size_t>(info.imageHeight) * 4;
            info.hasEmbeddedPreview = (info.imageBytes.size() == expectedSize);
            if (!info.hasEmbeddedPreview) {
                info.imageBytes.clear();
            }
        }

        return info;
    }
}

std::vector<IOPanelSceneTile> loadIOPanelSceneTiles(std::string_view scenesDirectory) {
    std::vector<IOPanelSceneTile> sceneTiles;

    const std::filesystem::path scenesDir = scenesDirectory.empty() ? std::filesystem::path(".") : std::filesystem::path(scenesDirectory);
    if (!std::filesystem::exists(scenesDir) || !std::filesystem::is_directory(scenesDir)) {
        return sceneTiles;
    }

    std::vector<std::filesystem::path> scenePaths;
    for (const auto& entry : std::filesystem::directory_iterator(scenesDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (entry.path().extension() == ".lat") {
            scenePaths.push_back(entry.path());
        }
    }

    std::sort(scenePaths.begin(), scenePaths.end());
    sceneTiles.reserve(scenePaths.size());

    for (const auto& path : scenePaths) {
        ParsedSceneInfo parsed = parseSceneInfo(path);

        IOPanelSceneTile tile;
        tile.path = path.string();
        tile.title = std::move(parsed.title);
        tile.description = std::move(parsed.description);

        if (parsed.hasEmbeddedPreview) {
            sf::Image image;
            image.resize({parsed.imageWidth, parsed.imageHeight}, parsed.imageBytes.data());
            tile.hasPreview = tile.previewTexture.loadFromImage(image);
        }

        sceneTiles.push_back(std::move(tile));
    }

    return sceneTiles;
}
