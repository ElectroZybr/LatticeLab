#pragma once

struct StepContext;

class LangevinScheme {
public:
    void pipeline(StepContext& ctx) const;
};
