// #pragma once

// #include <cstdint>

// #include <Lattice/Tools/BmRunner/Metrics/Alloc.hpp>
// #include <Lattice/Tools/BmRunner/Stages.hpp>

// namespace Lattice::detail {

// class AllocationMetric : public Stage {
//     Alloc::Snapshot start_;
//     uint64_t allocations_ = 0;
//     uint64_t bytes_ = 0;
//     uint64_t iterations_ = 0;
//     Alloc::Snapshot last_;
//     size_t lastIterations_ = 0;

// protected:
//     double allocations() const noexcept {
//         return static_cast<double>(allocations_) /
//             static_cast<double>(iterations_);
//     }

//     double bytes() const noexcept {
//         return static_cast<double>(bytes_) /
//             static_cast<double>(iterations_);
//     }

//     double lastAllocations() const noexcept {
//         return static_cast<double>(last_.allocations) /
//             static_cast<double>(lastIterations_);
//     }

//     double lastBytes() const noexcept {
//         return static_cast<double>(last_.bytes) /
//             static_cast<double>(lastIterations_);
//     }

//     void reset() noexcept {
//         allocations_ = 0;
//         bytes_ = 0;
//         iterations_ = 0;
//     }

// public:
//     void begin(StageContext&) override {
//         start_ = Alloc::begin();
//     }

//     void end(StageContext& context) override {
//         const auto value = Alloc::end(start_);
//         last_ = value;
//         lastIterations_ = context.iterations;
//         allocations_ += value.allocations;
//         bytes_ += value.bytes;
//         iterations_ += context.iterations;
//     }
// };

// }
