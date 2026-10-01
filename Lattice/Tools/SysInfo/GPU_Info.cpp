#include "SystemInfo.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>

namespace Lattice::SystemInfo {

namespace {

namespace fs = std::filesystem;

std::string readText(const fs::path& path) {
    std::ifstream file(path);
    std::string value;

    if (!file || !std::getline(file, value))
        return {};

    while (!value.empty() && (value.back() == '\n' || value.back() == '\r' || value.back() == ' '))
        value.pop_back();

    return value;
}

uint64_t readUint(const fs::path& path) {
    const std::string value = readText(path);

    if (value.empty())
        return 0;

    uint64_t result = 0;
    const auto [_, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
    return ec == std::errc{} ? result : 0;
}

std::string driverName(const fs::path& device) {
    std::error_code ec;
    const fs::path driver = fs::read_symlink(device / "driver", ec);
    return ec ? std::string{} : driver.filename().string();
}

void enrichNvidia(GpuInfo& gpu) {
    if (gpu.driver != "nvidia")
        return;

    const fs::path info = fs::path("/proc/driver/nvidia/gpus") / gpu.pciAddress / "information";
    std::ifstream file(info);
    std::string line;

    while (std::getline(file, line)) {
        const size_t colon = line.find(':');

        if (colon == std::string::npos)
            continue;

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
            value.erase(value.begin());

        if (key == "Model")
            gpu.name = value;
    }
}

void enrichAmd(const fs::path& device, GpuInfo& gpu) {
    if (gpu.driver != "amdgpu")
        return;

    gpu.vramBytes = readUint(device / "mem_info_vram_total");
}

}

std::vector<GpuInfo> collectGpus() {
    std::vector<GpuInfo> result;

    const fs::path root = "/sys/class/drm";

    if (!fs::exists(root))
        return result;

    for (const auto& entry : fs::directory_iterator(root)) {
        const std::string card = entry.path().filename().string();

        if (!card.starts_with("card") || card.find('-') != std::string::npos)
            continue;

        const fs::path device = entry.path() / "device";

        if (!fs::exists(device))
            continue;

        std::error_code ec;
        const fs::path canonical = fs::canonical(device, ec);

        if (ec)
            continue;

        GpuInfo gpu;

        gpu.pciAddress = canonical.filename().string();
        gpu.driver = driverName(device);

        const std::string vendor = readText(device / "vendor");
        const std::string id = readText(device / "device");

        gpu.name = vendor + ":" + id;

        enrichAmd(device, gpu);
        enrichNvidia(gpu);

        result.push_back(std::move(gpu));
    }

    return result;
}

}