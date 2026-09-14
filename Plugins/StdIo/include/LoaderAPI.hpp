#pragma once

// #include "Lattice/Kernel/Node.hpp"

#include "Document.hpp"

class Value;

class LoaderAPI {
public:
    virtual ~LoaderAPI() = default;
    virtual void load(const Document& document) = 0;
};