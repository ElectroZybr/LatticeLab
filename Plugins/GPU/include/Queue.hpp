#pragma once

#include <Lattice/Kernel/Component.hpp>

#include "CommandList.hpp"

namespace GPU {

struct Queue : public Lattice::Component {
    virtual ~Queue() = default;

    virtual void submit(CommandList&) = 0;
    virtual void waitIdle() = 0;
};

}