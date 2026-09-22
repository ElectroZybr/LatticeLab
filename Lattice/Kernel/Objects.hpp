// #pragma once

// #include <cstdint>
// #include <string>
// #include <limits>
// #include <functional>


// namespace Lattice {

// using ObjectId = uint32_t;
// class Node;

// inline constexpr ObjectId InvalidObjectId = std::numeric_limits<ObjectId>::max();

// struct Object {
//     std::string name;
//     ObjectId parent = InvalidObjectId;
//     Node* node = nullptr;
//     bool exists = true;
// };

// struct ObjectKey {
//     std::string name;
//     ObjectId parent;

//     bool operator==(const ObjectKey&) const = default;
// };

// struct ObjectKeyHash {
//     size_t operator()(const ObjectKey& key) const noexcept {
//         size_t h = std::hash<ObjectId>{}(key.parent);
//         h ^= std::hash<std::string>{}(key.name)
//             + 0x9e3779b9 + (h << 6) + (h >> 2);
//         return h;
//     }
// };

// }
