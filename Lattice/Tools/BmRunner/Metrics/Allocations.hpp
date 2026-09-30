// #pragma once

// #include <Lattice/Tools/BmRunner/Metrics/AllocationMetric.hpp>

// namespace Lattice {

// class Allocations : public detail::AllocationMetric {
// public:
//     std::string_view name() const noexcept override {
//         return "Allocations";
//     }

//     void finish(StageContext& context) override {
//         context.result.add("allocations", allocations(), Unit::Count);
//         reset();
//     }

//     void progressMetrics(
//         const StageContext&,
//         std::vector<Metric>& metrics
//     ) const override {
//         metrics.push_back({
//             .name = "allocations",
//             .value = lastAllocations(),
//             .unit = Unit::Count
//         });
//     }
// };

// }
