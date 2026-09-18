#pragma once

#include <Lattice/Kernel/Component.hpp>


#include "Lattice/Kernel/Value.hpp"


class LoaderAPI : public Lattice::Component {
public:
    virtual ~LoaderAPI() = default;
    virtual std::string_view section() const = 0;
    virtual void load(const Lattice::Value& section) = 0;
};