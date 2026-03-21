#pragma once

struct StepContext;

class RK4Scheme {
public:
    void pipeline(StepContext& ctx) const;
};
