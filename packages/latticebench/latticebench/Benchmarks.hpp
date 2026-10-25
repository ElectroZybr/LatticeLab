#pragma once

#include <deque>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include <Lattice/Lattice.hpp>
#include <lbm-runner/Benchmarks.hpp>

/// Runs registered Lattice benchmarks and keeps their result tables.
/// Infrastructure object: actions are exported by composition.
class Benchmarks final {
public:
    /// Run one benchmark (or group) by name, or all of them.
    void run(Lattice::ActionContext& context, std::optional<std::string> name);
    void list(Lattice::ActionContext& context);
    /// Present a stored result table by its name.
    void view(Lattice::ActionContext& context, std::string name) const;

    [[nodiscard]] size_t resultCount() const noexcept { return results_.size(); }

private:
    void onProgress(const Lattice::Benchmarks::SampleResult& progress);
    void writeResults(
        Lattice::ActionContext& context,
        std::string_view title,
        std::span<const Lattice::Benchmarks::BenchResult> results
    );

    std::deque<std::pair<std::string, Lattice::BasicTable>> results_;
    size_t nextResult_ = 1;
};
