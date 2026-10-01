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

std::vector<uint32_t> parseCpuList(std::string_view value) {
    std::vector<uint32_t> result;

    while (!value.empty()) {
        const size_t comma = value.find(',');
        const std::string_view part = value.substr(0, comma);
        const size_t dash = part.find('-');

        if (dash == std::string_view::npos) {
            uint32_t cpu = 0;

            if (std::from_chars(part.data(), part.data() + part.size(), cpu).ec == std::errc{})
                result.push_back(cpu);
        } else {
            uint32_t first = 0, last = 0;

            const auto a = part.substr(0, dash);
            const auto b = part.substr(dash + 1);

            if (
                std::from_chars(a.data(), a.data() + a.size(), first).ec == std::errc{} &&
                std::from_chars(b.data(), b.data() + b.size(), last).ec == std::errc{}
            ) {
                for (uint32_t cpu = first; cpu <= last; ++cpu)
                    result.push_back(cpu);
            }
        }

        if (comma == std::string_view::npos)
            break;

        value.remove_prefix(comma + 1);
    }

    return result;
}

uint64_t parseKbLine(std::string_view line) {
    const size_t colon = line.find(':');

    if (colon == std::string_view::npos)
        return 0;

    line.remove_prefix(colon + 1);

    while (!line.empty() && line.front() == ' ')
        line.remove_prefix(1);

    uint64_t kb = 0;
    const auto [_, ec] = std::from_chars(line.data(), line.data() + line.size(), kb);

    return ec == std::errc{} ? kb * 1024ull : 0;
}

MemoryInfo collectGlobalMemory() {
    MemoryInfo info;

    std::ifstream file("/proc/meminfo");
    std::string line;

    while (std::getline(file, line)) {
        if (line.starts_with("MemTotal:"))
            info.totalBytes = parseKbLine(line);
        else if (line.starts_with("MemAvailable:"))
            info.availableBytes = parseKbLine(line);
    }

    return info;
}

void collectNumaNodes(MemoryInfo& info) {
    const fs::path root = "/sys/devices/system/node";

    if (!fs::exists(root))
        return;

    for (const auto& entry : fs::directory_iterator(root)) {
        if (!entry.is_directory())
            continue;

        const std::string name = entry.path().filename().string();

        if (!name.starts_with("node") || name.size() <= 4)
            continue;

        uint32_t nodeId = 0;

        if (std::from_chars(name.data() + 4, name.data() + name.size(), nodeId).ec != std::errc{})
            continue;

        MemoryNodeInfo node;
        node.id = nodeId;
        node.logicalCpus = parseCpuList(readText(entry.path() / "cpulist"));

        std::ifstream file(entry.path() / "meminfo");
        std::string line;

        while (std::getline(file, line)) {
            if (line.find("MemTotal:") != std::string::npos)
                node.totalBytes = parseKbLine(line);
            else if (line.find("MemFree:") != std::string::npos)
                node.freeBytes = parseKbLine(line);
        }

        info.nodes.push_back(std::move(node));
    }
}

}

MemoryInfo collectMemory() {
    MemoryInfo info = collectGlobalMemory();
    collectNumaNodes(info);
    return info;
}

}