#include "SystemInfo.hpp"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

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
    uint64_t result = 0;

    if (value.empty())
        return 0;

    const auto [_, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
    return ec == std::errc{} ? result : 0;
}

uint64_t parseSize(std::string_view value) {
    if (value.empty())
        return 0;

    uint64_t multiplier = 1;

    switch (value.back()) {
        case 'K': multiplier = 1024ull; value.remove_suffix(1); break;
        case 'M': multiplier = 1024ull * 1024ull; value.remove_suffix(1); break;
        case 'G': multiplier = 1024ull * 1024ull * 1024ull; value.remove_suffix(1); break;
    }

    uint64_t result = 0;
    const auto [_, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
    return ec == std::errc{} ? result * multiplier : 0;
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

CpuArch architecture() {
#if defined(__x86_64__)
    return CpuArch::X86_64;
#elif defined(__aarch64__)
    return CpuArch::AArch64;
#else
    return CpuArch::Unknown;
#endif
}

CpuFeature parseFeatures(std::string_view text) {
    CpuFeature features = CpuFeature::None;
    std::istringstream stream{std::string(text)};
    std::string flag;

    while (stream >> flag) {
        if (flag == "sse") features |= CpuFeature::SSE;
        else if (flag == "sse2") features |= CpuFeature::SSE2;
        else if (flag == "pni") features |= CpuFeature::SSE3;
        else if (flag == "ssse3") features |= CpuFeature::SSSE3;
        else if (flag == "sse4_1") features |= CpuFeature::SSE41;
        else if (flag == "sse4_2") features |= CpuFeature::SSE42;
        else if (flag == "avx") features |= CpuFeature::AVX;
        else if (flag == "avx2") features |= CpuFeature::AVX2;
        else if (flag == "fma") features |= CpuFeature::FMA;
        else if (flag == "avx512f") features |= CpuFeature::AVX512F;
        else if (flag == "asimd") features |= CpuFeature::NEON;
        else if (flag == "sve") features |= CpuFeature::SVE;
    }

    return features;
}

CacheType parseCacheType(std::string_view value) {
    if (value == "Data") return CacheType::Data;
    if (value == "Instruction") return CacheType::Instruction;
    return CacheType::Unified;
}

bool sameCache(const CacheInfo& a, const CacheInfo& b) {
    return
        a.level == b.level &&
        a.type == b.type &&
        a.sharedLogicalCpus == b.sharedLogicalCpus;
}

struct ProcCpuInfo {
    uint32_t id = 0;
    std::string name;
    CpuFeature features = CpuFeature::None;
};

std::vector<ProcCpuInfo> readProcCpuInfo() {
    std::ifstream file("/proc/cpuinfo");
    std::vector<ProcCpuInfo> result;

    ProcCpuInfo current;
    bool active = false;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) {
            if (active)
                result.push_back(std::move(current));

            current = {};
            active = false;
            continue;
        }

        const size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        auto trim = [](std::string value) {
            const size_t first = value.find_first_not_of(" \t");
            const size_t last = value.find_last_not_of(" \t");
            return first == std::string::npos ? std::string{} : value.substr(first, last - first + 1);
        };

        const std::string key = trim(line.substr(0, colon));
        const std::string value = trim(line.substr(colon + 1));

        if (key == "processor") {
            current.id = static_cast<uint32_t>(std::stoul(value));
            active = true;
        } else if (key == "model name" || key == "Processor") {
            current.name = value;
        } else if (key == "flags" || key == "Features") {
            current.features = parseFeatures(value);
        }
    }

    if (active)
        result.push_back(std::move(current));

    return result;
}

}

std::vector<ProcessorInfo> collectCPU() {
    std::vector<ProcessorInfo> processors;
    std::map<uint32_t, size_t> packageIndices;

    const fs::path root = "/sys/devices/system/cpu";

    for (const auto& entry : fs::directory_iterator(root)) {
        const std::string name = entry.path().filename().string();

        if (!entry.is_directory() || !name.starts_with("cpu") || name.size() <= 3)
            continue;

        uint32_t logicalCpu = 0;
        if (std::from_chars(name.data() + 3, name.data() + name.size(), logicalCpu).ec != std::errc{})
            continue;

        const fs::path topology = entry.path() / "topology";

        const uint32_t packageId = static_cast<uint32_t>(readUint(topology / "physical_package_id"));
        const uint32_t coreId = static_cast<uint32_t>(readUint(topology / "core_id"));

        size_t index = 0;

        if (auto it = packageIndices.find(packageId); it != packageIndices.end()) {
            index = it->second;
        } else {
            index = processors.size();
            packageIndices.emplace(packageId, index);

            processors.push_back({
                .id = packageId,
                .arch = architecture()
            });
        }

        ProcessorInfo& processor = processors[index];

        auto core = std::find_if(
            processor.cores.begin(),
            processor.cores.end(),
            [coreId](const CpuCoreInfo& value) {
                return value.id == coreId;
            }
        );

        if (core == processor.cores.end())
            processor.cores.push_back({.id = coreId, .logicalCpus = {logicalCpu}});
        else
            core->logicalCpus.push_back(logicalCpu);
    }

    const auto procInfo = readProcCpuInfo();

    for (ProcessorInfo& processor : processors) {
        if (processor.cores.empty() || processor.cores.front().logicalCpus.empty())
            continue;

        const uint32_t representativeCpu = processor.cores.front().logicalCpus.front();

        const auto info = std::find_if(
            procInfo.begin(),
            procInfo.end(),
            [representativeCpu](const ProcCpuInfo& value) {
                return value.id == representativeCpu;
            }
        );

        if (info != procInfo.end()) {
            processor.name = info->name;
            processor.features = info->features;
        }

        for (const CpuCoreInfo& core : processor.cores) {
            for (uint32_t cpu : core.logicalCpus) {
                const fs::path cpuRoot = root / ("cpu" + std::to_string(cpu));

                const uint64_t minHz = readUint(cpuRoot / "cpufreq/cpuinfo_min_freq") * 1000ull;
                const uint64_t maxHz = readUint(cpuRoot / "cpufreq/cpuinfo_max_freq") * 1000ull;

                if (minHz && (!processor.minFrequencyHz || minHz < processor.minFrequencyHz))
                    processor.minFrequencyHz = minHz;

                processor.maxFrequencyHz = std::max(processor.maxFrequencyHz, maxHz);

                const fs::path cacheRoot = cpuRoot / "cache";
                if (!fs::exists(cacheRoot))
                    continue;

                for (const auto& cacheEntry : fs::directory_iterator(cacheRoot)) {
                    if (!cacheEntry.is_directory())
                        continue;

                    const std::string cacheName = cacheEntry.path().filename().string();

                    if (!cacheName.starts_with("index"))
                        continue;

                    CacheInfo cache{
                        .level = static_cast<uint8_t>(readUint(cacheEntry.path() / "level")),
                        .type = parseCacheType(readText(cacheEntry.path() / "type")),
                        .sizeBytes = parseSize(readText(cacheEntry.path() / "size")),
                        .lineSizeBytes = static_cast<uint32_t>(readUint(cacheEntry.path() / "coherency_line_size")),
                        .ways = static_cast<uint32_t>(readUint(cacheEntry.path() / "ways_of_associativity")),
                        .sets = static_cast<uint32_t>(readUint(cacheEntry.path() / "number_of_sets")),
                        .sharedLogicalCpus = parseCpuList(readText(cacheEntry.path() / "shared_cpu_list"))
                    };

                    std::ranges::sort(cache.sharedLogicalCpus);

                    if (std::ranges::find_if(
                        processor.caches,
                        [&](const CacheInfo& other) {
                            return sameCache(cache, other);
                        }
                    ) == processor.caches.end()) {
                        processor.caches.push_back(std::move(cache));
                    }
                }
            }
        }
    }

    return processors;
}

}