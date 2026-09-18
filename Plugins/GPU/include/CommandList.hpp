#pragma once


namespace GPU {

class CommandList {
public:
    virtual ~CommandList() = default;
    virtual void submit() = 0;
};

}