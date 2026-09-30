// #pragma once

// #include <Lattice/Tools/BmRunner/Metrics/AllocationMetric.hpp>

// namespace Lattice {

// class Memory : public detail::AllocationMetric {
// public:
//     std::string_view name() const noexcept override {
//         return "Memory";
//     }

//     void finish(StageContext& context) override {
//         context.result.add("memory", bytes(), Unit::Bytes);
//         reset();
//     }

//     void progressMetrics(
//         const StageContext&,
//         std::vector<Metric>& metrics
//     ) const override {
//         metrics.push_back({
//             .name = "memory",
//             .value = lastBytes(),
//             .unit = Unit::Bytes
//         });
//     }
// };

// }
