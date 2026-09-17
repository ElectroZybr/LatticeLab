#pragma once

#include "CommandList.hpp"

namespace GPU {

struct Queue {
    virtual ~Queue() = default;

    virtual void submit(CommandList&) = 0;
    virtual void waitIdle() = 0;
};

}