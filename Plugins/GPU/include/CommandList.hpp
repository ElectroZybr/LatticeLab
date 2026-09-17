#pragma once

namespace GPU {

struct CommandList {
    virtual ~CommandList() = default;

    virtual void begin() = 0;
    virtual void end() = 0;
};

}