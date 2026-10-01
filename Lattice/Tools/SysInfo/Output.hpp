#pragma once

#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Tools/Logger.hpp>
#include <Lattice/Tools/SysInfo/SystemInfo.hpp>
#include <Lattice/Tools/TextFormatter.hpp>

namespace Lattice::SystemInfo::Output {

namespace {

struct CacheGroup {
    uint8_t level = 0;
    CacheType type = CacheType::Unified;
    uint64_t sizeBytes = 0;
    uint32_t lineSizeBytes = 0;
    uint32_t ways = 0;
    size_t count = 0;
    std::vector<uint32_t> logicalCpus;
};

std::string formatBytes(uint64_t bytes) {
    constexpr double KiB = 1024.0;
    constexpr double MiB = KiB * 1024.0;
    constexpr double GiB = MiB * 1024.0;
    const double value = static_cast<double>(bytes);

    if (value >= GiB)
        return std::format("{:.2f} GiB", value / GiB);
    if (value >= MiB)
        return std::format("{:.2f} MiB", value / MiB);
    if (value >= KiB)
        return std::format("{:.2f} KiB", value / KiB);

    return std::format("{} B", bytes);
}

std::string formatFrequency(uint64_t hz) {
    if (hz >= 1'000'000'000ull)
        return std::format("{:.2f} GHz", hz / 1e9);
    if (hz >= 1'000'000ull)
        return std::format("{:.2f} MHz", hz / 1e6);

    return std::format("{} Hz", hz);
}

std::string compilerName(Compiler compiler) {
    switch (compiler) {
        case Compiler::GCC:   return "GCC";
        case Compiler::Clang: return "Clang";
        case Compiler::MSVC:  return "MSVC";
        case Compiler::Unknown: break;
    }

    return "unknown";
}

std::string buildTypeName(BuildType build) {
    switch (build) {
        case BuildType::Debug:   return "debug";
        case BuildType::Release: return "release";
        case BuildType::Unknown: break;
    }

    return "unknown";
}

std::string formatVersion(uint32_t major, uint32_t minor, uint32_t patch) {
    return std::format("{}.{}.{}", major, minor, patch);
}

std::string formatCpuList(const std::vector<uint32_t>& cpus) {
    std::string result;

    for (size_t i = 0; i < cpus.size(); ++i) {
        if (i != 0)
            result += ',';

        result += std::format("{}", cpus[i]);
    }

    return result;
}

constexpr uint8_t cacheTypeOrder(CacheType type) noexcept {
    switch (type) {
        case CacheType::Data:        return 0;
        case CacheType::Instruction: return 1;
        case CacheType::Unified:     return 2;
    }

    return 3;
}

std::vector<CacheGroup> groupCaches(const ProcessorInfo& cpu) {
    std::vector<CacheGroup> groups;

    for (const CacheInfo& cache : cpu.caches) {
        auto found = std::find_if(
            groups.begin(),
            groups.end(),
            [&](const CacheGroup& group) {
                return
                    group.level == cache.level &&
                    group.type == cache.type &&
                    group.sizeBytes == cache.sizeBytes &&
                    group.lineSizeBytes == cache.lineSizeBytes &&
                    group.ways == cache.ways;
            }
        );

        if (found == groups.end()) {
            groups.push_back({
                .level = cache.level,
                .type = cache.type,
                .sizeBytes = cache.sizeBytes,
                .lineSizeBytes = cache.lineSizeBytes,
                .ways = cache.ways,
                .count = 1,
                .logicalCpus = cache.sharedLogicalCpus
            });
            continue;
        }

        ++found->count;
        found->logicalCpus.insert(
            found->logicalCpus.end(),
            cache.sharedLogicalCpus.begin(),
            cache.sharedLogicalCpus.end()
        );
    }

    for (CacheGroup& group : groups) {
        std::ranges::sort(group.logicalCpus);
        group.logicalCpus.erase(
            std::unique(group.logicalCpus.begin(), group.logicalCpus.end()),
            group.logicalCpus.end()
        );
    }

    std::ranges::sort(
        groups,
        [](const CacheGroup& a, const CacheGroup& b) {
            if (a.level != b.level)
                return a.level < b.level;

            const uint8_t aType = cacheTypeOrder(a.type);
            const uint8_t bType = cacheTypeOrder(b.type);

            if (aType != bType)
                return aType < bType;
            if (a.sizeBytes != b.sizeBytes)
                return a.sizeBytes < b.sizeBytes;
            if (a.ways != b.ways)
                return a.ways < b.ways;

            return a.lineSizeBytes < b.lineSizeBytes;
        }
    );

    return groups;
}

}

inline TextFormatter format(const MachineInfo& info) {
    TextFormatter output;
    bool first = true;

    const auto appendLine = [&](const TextFormatter& line) {
        if (!first)
            output.append("\n");
        first = false;
        output.append(line);
    };

    TextFormatter system = TextFormatter::format(
        "<a2><b>{:<9}<//> <light>{}</>",
        "System",
        info.os.name
    );

    if (!info.os.kernel.empty())
        system += TextFormatter::format("  <mut>kernel: {}</>", info.os.kernel);

    if (!info.os.architecture.empty())
        system += TextFormatter::format("  <mut>arch: {}</>", info.os.architecture);

    appendLine(system);
    appendLine(TextFormatter::format(
        "<a2><b>{:<9}<//> <light>{} {}</>  "
        "<mut>compiler:</> <light>{} {}</>  <mut>type:</> <light>{}</>",
        "Build",
        info.build.executable.empty() ? "unknown" : info.build.executable,
        formatVersion(
            info.build.versionMajor,
            info.build.versionMinor,
            info.build.versionPatch
        ),
        compilerName(info.build.compiler),
        formatVersion(
            info.build.compilerMajor,
            info.build.compilerMinor,
            info.build.compilerPatch
        ),
        buildTypeName(info.build.build)
    ));
    appendLine(TextFormatter::format(
        "<a2><b>{:<9}<//> <light>{}</> <mut>total,</> "
        "<light>{}</> <mut>available</>",
        "Memory",
        formatBytes(info.memory.totalBytes),
        formatBytes(info.memory.availableBytes)
    ));

    for (const MemoryNodeInfo& node : info.memory.nodes) {
        appendLine(TextFormatter::format(
            "  <mut>NUMA {}</>  <light>{}</>  <mut>CPUs</>={}",
            node.id,
            formatBytes(node.totalBytes),
            formatCpuList(node.logicalCpus)
        ));
    }

    for (const ProcessorInfo& cpu : info.processors) {
        size_t threads = 0;

        for (const CpuCoreInfo& core : cpu.cores)
            threads += core.logicalCpus.size();

        appendLine(TextFormatter::format(
            "<a2><b>CPU {:<5}<//> <light>{}</>  {} <mut>cores /</> "
            "{} <mut>threads</>  <light>{}-{}</>",
            cpu.id,
            cpu.name,
            cpu.cores.size(),
            threads,
            formatFrequency(cpu.minFrequencyHz),
            formatFrequency(cpu.maxFrequencyHz)
        ));

        for (const CacheGroup& cache : groupCaches(cpu)) {
            const std::string_view type =
                cache.type == CacheType::Data ? "D" :
                cache.type == CacheType::Instruction ? "I" : "";
            const std::string cacheName =
                std::format("L{}{}", cache.level, type);

            appendLine(TextFormatter::format(
                "  <mut>{:<7} <light>{:<10} x{:<2}</>  {:>2}-way  line={:<2} B  "
                "<mut>CPUs</>={}</>",
                cacheName,
                formatBytes(cache.sizeBytes),
                cache.count,
                cache.ways,
                cache.lineSizeBytes,
                cache.logicalCpus.size()
            ));
        }
    }

    for (size_t i = 0; i < info.gpus.size(); ++i) {
        const GpuInfo& gpu = info.gpus[i];
        TextFormatter line = TextFormatter::format(
            "<a2><b>GPU {:<5}<//> <light>{}</>  <mut>driver</>={}  "
            "<mut>pci</>={}",
            i,
            gpu.name,
            gpu.driver,
            gpu.pciAddress
        );

        if (gpu.vramBytes) {
            line += TextFormatter::format(
                "  <mut>VRAM</>={}",
                formatBytes(gpu.vramBytes)
            );
        }

        appendLine(line);
    }

    return output;
}

inline void print(const MachineInfo& info) {
    Logger::blank();
    Logger::message(format(info));
    Logger::blank();
}

inline void print() {
    print(collectMachineInfo());
}

}
