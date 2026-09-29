#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Lattice {

enum class Unit : uint8_t {
    None,
    Count,
    Nanoseconds,
    Bytes,
    Ratio,
    Percent,
    Cycles
};

struct Metric {
    std::string name;
    double value = 0;
    Unit unit = Unit::None;
};

struct Result {
    std::vector<Metric> metrics;

    void add(std::string name, double value, Unit unit = Unit::None) {
        metrics.push_back({
            .name = std::move(name),
            .value = value,
            .unit = unit
        });
    }

    const Metric* find(std::string_view name) const noexcept {
        for (const auto& metric : metrics)
            if (metric.name == name)
                return &metric;

        return nullptr;
    }
};

struct Progress {
    std::string_view name;
    std::string_view group;
    std::string_view stage;
    size_t current = 0;
    size_t total = 0;
    std::vector<Metric> metrics;
};

using ProgressCallback = std::function<void(const Progress&)>;

}