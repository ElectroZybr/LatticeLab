// #pragma once

// #include <cstdint>
// #include <functional>
// #include <vector>

// #include "Lattice/Kernel/Objects.hpp"

// namespace Lattice {

// enum class ChoiceMode : uint8_t {
//     Slot,
//     Focus,
//     Custom
// };

// struct Choice {
//     ChoiceMode mode = ChoiceMode::Custom;
//     ObjectId selected = InvalidObjectId;
//     std::function<void(ObjectId)> select;

//     bool choose(uint32_t index) {
//         if (index >= options.size() || !select) return false;
//         selected = options[index];
//         select(selected);
//         return true;
//     }
// };

// }