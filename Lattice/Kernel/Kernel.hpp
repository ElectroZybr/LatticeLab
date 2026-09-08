#pragma once

#include <Lattice/Kernel/Blueprints.hpp>
#include <Lattice/Kernel/Settings.hpp>


namespace Lattice {
struct Kernel {
    Blueprints blueprints;
    Objects       objects;
    Settings     settings;
    Context       context;
};

}