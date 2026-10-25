#pragma once

#include <string_view>


#include <Lattice/kernel/Value.hpp>


/// Domain loader contract; the owning domain explicitly creates and invokes it.
class LoaderAPI {
public:
    virtual ~LoaderAPI() = default;
    virtual std::string_view section() const = 0;
    virtual void load(const Lattice::Value& section) = 0;
};
