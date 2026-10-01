#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Lattice::SystemInfo {

enum class CpuArch : uint8_t {
    Unknown,
    X86_64,
    AArch64
};

enum class Compiler : uint8_t {
    Unknown,
    GCC,
    Clang,
    MSVC
};

enum class BuildType : uint8_t {
    Unknown,
    Debug,
    Release
};

enum class CacheType : uint8_t {
    Data,
    Instruction,
    Unified
};

enum class CpuFeature : uint64_t {
    None   = 0,

    SSE    = 1ull << 0,
    SSE2   = 1ull << 1,
    SSE3   = 1ull << 2,
    SSSE3  = 1ull << 3,
    SSE41  = 1ull << 4,
    SSE42  = 1ull << 5,

    AVX    = 1ull << 6,
    AVX2   = 1ull << 7,
    FMA    = 1ull << 8,

    AVX512F = 1ull << 9,

    NEON   = 1ull << 10,
    SVE    = 1ull << 11
};

constexpr CpuFeature operator|(CpuFeature a, CpuFeature b) noexcept {
    return static_cast<CpuFeature>(
        static_cast<uint64_t>(a) |
        static_cast<uint64_t>(b)
    );
}

constexpr CpuFeature operator&(CpuFeature a, CpuFeature b) noexcept {
    return static_cast<CpuFeature>(
        static_cast<uint64_t>(a) &
        static_cast<uint64_t>(b)
    );
}

constexpr CpuFeature& operator|=(CpuFeature& a, CpuFeature b) noexcept {
    a = a | b;
    return a;
}

constexpr bool hasFeature(CpuFeature features, CpuFeature feature) noexcept {
    return (features & feature) == feature;
}

struct BuildInfo {
    std::string executable;

    uint32_t versionMajor = 0;
    uint32_t versionMinor = 0;
    uint32_t versionPatch = 0;

    Compiler compiler = Compiler::Unknown;

    uint32_t compilerMajor = 0;
    uint32_t compilerMinor = 0;
    uint32_t compilerPatch = 0;

    BuildType build = BuildType::Unknown;
};

struct OsInfo {
    std::string name;
    std::string kernel;
    std::string architecture;
};

struct CacheInfo {
    uint8_t level = 0;
    CacheType type = CacheType::Unified;

    uint64_t sizeBytes = 0;
    uint32_t lineSizeBytes = 0;
    uint32_t ways = 0;
    uint32_t sets = 0;

    std::vector<uint32_t> sharedLogicalCpus;
};

struct CpuCoreInfo {
    uint32_t id = 0;
    std::vector<uint32_t> logicalCpus;
};

struct ProcessorInfo {
    uint32_t id = 0;

    std::string name;
    CpuArch arch = CpuArch::Unknown;
    CpuFeature features = CpuFeature::None;

    uint64_t minFrequencyHz = 0;
    uint64_t maxFrequencyHz = 0;

    std::vector<CpuCoreInfo> cores;
    std::vector<CacheInfo> caches;
};

struct MemoryNodeInfo {
    uint32_t id = 0;
    uint64_t totalBytes = 0;
    uint64_t freeBytes = 0;
    std::vector<uint32_t> logicalCpus;
};

struct MemoryInfo {
    uint64_t totalBytes = 0;
    uint64_t availableBytes = 0;

    std::vector<MemoryNodeInfo> nodes;
};

struct GpuInfo {
    std::string name;
    std::string driver;
    std::string pciAddress;
    std::string computeInfo;

    uint64_t vramBytes = 0;
};

struct MachineInfo {
    OsInfo os;
    BuildInfo build;

    MemoryInfo memory;
    std::vector<ProcessorInfo> processors;
    std::vector<GpuInfo> gpus;
};

std::vector<ProcessorInfo> collectCPU();
std::vector<GpuInfo> collectGpus();
MemoryInfo collectMemory();
OsInfo collectOS();
BuildInfo collectBuild();

inline MachineInfo collectMachineInfo() {
    MachineInfo info;

    info.os = collectOS();
    info.build = collectBuild();
    info.processors = collectCPU();
    info.memory = collectMemory();
    info.gpus = collectGpus();

    return info;
}

}
