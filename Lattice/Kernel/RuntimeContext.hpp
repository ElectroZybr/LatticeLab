#pragma once

#include <memory>

#include <Lattice/Kernel/Settings.hpp>
#include "Lattice/Kernel/Objects.hpp"


namespace Lattice {

class Node;

inline constexpr std::string_view DefaultBlueprintsPath = "Blueprints";
inline constexpr std::string_view DefaultInstanceName = "default";

struct Meta {
    void* (*create)(Node&) = nullptr;
    void (*destroy)(Node&) = nullptr;
    void (*configure)(Node&) = nullptr;
};

struct Primitives {
    ObjectId action;
    ObjectId  param;
};

struct RuntimeContext {
    Objects objects;
    Settings settings;
    Context context;
    Primitives primitives;
    std::vector<std::unique_ptr<Meta>> metas;
};

}