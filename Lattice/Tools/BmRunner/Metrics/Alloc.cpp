// #include "Alloc.hpp"

// #include <cstdlib>
// #include <new>

// namespace {

// thread_local unsigned trackingDepth = 0;
// thread_local Lattice::Alloc::Snapshot totals;

// void* allocate(size_t size) {
//     size = size == 0 ? 1 : size;

//     while (true) {
//         if (void* ptr = std::malloc(size)) {
//             Lattice::Alloc::record(size);
//             return ptr;
//         }

//         const std::new_handler handler = std::get_new_handler();
//         if (!handler)
//             throw std::bad_alloc();

//         handler();
//     }
// }

// void* allocateAligned(size_t size, size_t alignment) {
//     size = size == 0 ? 1 : size;

//     while (true) {
//         void* ptr = nullptr;
//         if (::posix_memalign(&ptr, alignment, size) == 0) {
//             Lattice::Alloc::record(size);
//             return ptr;
//         }

//         const std::new_handler handler = std::get_new_handler();
//         if (!handler)
//             throw std::bad_alloc();

//         handler();
//     }
// }

// }

// namespace Lattice::Alloc {

// Snapshot begin() noexcept {
//     ++trackingDepth;
//     return totals;
// }

// Snapshot end(Snapshot start) noexcept {
//     if (trackingDepth != 0)
//         --trackingDepth;

//     return {
//         .allocations = totals.allocations - start.allocations,
//         .bytes = totals.bytes - start.bytes
//     };
// }

// void record(size_t bytes) noexcept {
//     if (trackingDepth == 0)
//         return;

//     ++totals.allocations;
//     totals.bytes += static_cast<uint64_t>(bytes);
// }

// }

// void* operator new(size_t size) {
//     return allocate(size);
// }

// void* operator new[](size_t size) {
//     return allocate(size);
// }

// void* operator new(size_t size, std::align_val_t alignment) {
//     return allocateAligned(size, static_cast<size_t>(alignment));
// }

// void* operator new[](size_t size, std::align_val_t alignment) {
//     return allocateAligned(size, static_cast<size_t>(alignment));
// }

// void* operator new(size_t size, const std::nothrow_t&) noexcept {
//     try { return allocate(size); } catch (...) { return nullptr; }
// }

// void* operator new[](size_t size, const std::nothrow_t&) noexcept {
//     try { return allocate(size); } catch (...) { return nullptr; }
// }

// void* operator new(
//     size_t size,
//     std::align_val_t alignment,
//     const std::nothrow_t&
// ) noexcept {
//     try { return allocateAligned(size, static_cast<size_t>(alignment)); }
//     catch (...) { return nullptr; }
// }

// void* operator new[](
//     size_t size,
//     std::align_val_t alignment,
//     const std::nothrow_t&
// ) noexcept {
//     try { return allocateAligned(size, static_cast<size_t>(alignment)); }
//     catch (...) { return nullptr; }
// }

// void operator delete(void* ptr) noexcept { std::free(ptr); }
// void operator delete[](void* ptr) noexcept { std::free(ptr); }
// void operator delete(void* ptr, size_t) noexcept { std::free(ptr); }
// void operator delete[](void* ptr, size_t) noexcept { std::free(ptr); }
// void operator delete(void* ptr, std::align_val_t) noexcept { std::free(ptr); }
// void operator delete[](void* ptr, std::align_val_t) noexcept { std::free(ptr); }
// void operator delete(void* ptr, size_t, std::align_val_t) noexcept { std::free(ptr); }
// void operator delete[](void* ptr, size_t, std::align_val_t) noexcept { std::free(ptr); }
// void operator delete(void* ptr, const std::nothrow_t&) noexcept { std::free(ptr); }
// void operator delete[](void* ptr, const std::nothrow_t&) noexcept { std::free(ptr); }
// void operator delete(
//     void* ptr,
//     std::align_val_t,
//     const std::nothrow_t&
// ) noexcept { std::free(ptr); }
// void operator delete[](
//     void* ptr,
//     std::align_val_t,
//     const std::nothrow_t&
// ) noexcept { std::free(ptr); }
