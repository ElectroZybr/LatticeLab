#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace Lattice {

class Perf {
    static constexpr size_t CounterCount = 6;

    int leader_ = -1;
    std::array<int, CounterCount> fds_{-1, -1, -1, -1, -1, -1};

public:
    struct Result {
        uint64_t cycles = 0;
        uint64_t instructions = 0;
        uint64_t cacheReferences = 0;
        uint64_t cacheMisses = 0;
        uint64_t branches = 0;
        uint64_t branchMisses = 0;

        double instructionPerСycles() const noexcept {
            return cycles
                ? static_cast<double>(instructions) / static_cast<double>(cycles)
                : 0.0;
        }

        double cyclesPerInstruction() const noexcept {
            return instructions
                ? static_cast<double>(cycles) / static_cast<double>(instructions)
                : 0.0;
        }

        double cacheMissRate() const noexcept {
            return cacheReferences
                ? static_cast<double>(cacheMisses) / static_cast<double>(cacheReferences)
                : 0.0;
        }

        double cacheHitRate() const noexcept {
            return cacheReferences
                ? 1.0 - cacheMissRate()
                : 0.0;
        }

        double branchMissRate() const noexcept {
            return branches
                ? static_cast<double>(branchMisses) / static_cast<double>(branches)
                : 0.0;
        }

        double branchHitRate() const noexcept {
            return branches
                ? 1.0 - branchMissRate()
                : 0.0;
        }

        double instructionsPerBranch() const noexcept {
            return branches
                ? static_cast<double>(instructions) / static_cast<double>(branches)
                : 0.0;
        }

        double missesPerKiloInstruction() const noexcept {
            return instructions
                ? static_cast<double>(cacheMisses) * 1000.0 /
                static_cast<double>(instructions)
                : 0.0;
        }
    };

    Perf();
    ~Perf();

    Perf(const Perf&) = delete;
    Perf& operator=(const Perf&) = delete;

    void start();
    Result stop();

    bool available() const noexcept { return leader_ >= 0; }

private:
    int openCounter(uint64_t config, int group);
    void close();
};

}