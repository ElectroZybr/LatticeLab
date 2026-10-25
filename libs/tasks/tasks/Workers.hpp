#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "Pool.hpp"

namespace Tasks {

/// Threads as an execution axis of benchmarks: the shape of a BmRunner axis (`Value`,
/// `name`, `values()`, `sweep()`, `label()`), with no dependency on BmRunner. A CPU
/// implementation declares it with `impl.axis<Tasks::Workers>()` and reads it with
/// `run.value<Tasks::Workers>()`; `--sweep workers` measures 1, 2, 4, … all threads.
struct Workers {
    using Value = size_t;
    static constexpr std::string_view name = "workers";

    static size_t all() { return std::max(1u, std::thread::hardware_concurrency()); }
    /// One thread and all of them: enough to compare with other implementations.
    static std::vector<size_t> values() { return all() == 1 ? std::vector<size_t>{1} : std::vector<size_t>{1, all()}; }
    static std::vector<size_t> sweep() {
        std::vector<size_t> values;
        for (size_t n = 1; n < all(); n *= 2)
            values.push_back(n);
        values.push_back(all());
        return values;
    }
    /// `cpu` for one thread, `cpu×16` for sixteen.
    static std::string label(size_t n) { return n == 1 ? std::string{} : "×" + std::to_string(n); }
};

/// The pool of every benchmark of the process: one set of threads, not one per bench.
inline Pool& benchPool() {
    static Pool pool;
    return pool;
}

}
