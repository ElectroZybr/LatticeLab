#pragma once

#include <Lattice/Kernel/Component.hpp>

namespace GPU {

struct CommandList : public Lattice::Component {
    virtual ~CommandList() = default;

    virtual void begin() = 0;
    virtual void end() = 0;
};

}