#pragma once

#include <Lattice/Kernel/Registry.hpp>
// #include <Lattice/Kernel/ObjectRegistry.hpp>
#include <Lattice/Kernel/Settings.hpp>

namespace Lattice {

// struct Context {
//     std::unordered_map<std::string, ObjectId> chain;
// };

// using Context = std::unordered_map<std::string, ObjectId>;

struct Kernel {
    Registry       registry;
    ObjectRegistry objects;
    Settings       settings;
    Context context;
};

}