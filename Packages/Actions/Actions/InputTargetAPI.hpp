#pragma once

/// An object that receives keybinds. Which one is active is a Context choice
/// (Focus<InputTargetAPI>); the router resolves bindings from that node.
class InputTargetAPI {
public:
    virtual ~InputTargetAPI() = default;
};
