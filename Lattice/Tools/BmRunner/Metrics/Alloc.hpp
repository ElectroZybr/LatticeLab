// #pragma once

// #include <cstddef>
// #include <cstdint>

// namespace Lattice::Alloc {

// struct Snapshot {
//     uint64_t allocations = 0;
//     uint64_t bytes = 0;
// };

// // Tracking is local to the current thread and supports nested measurements.
// Snapshot begin() noexcept;
// Snapshot end(Snapshot start) noexcept;
// void record(size_t bytes) noexcept;

// }
