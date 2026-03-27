#pragma once

#include "../utils/RateCounter.h"

class IntegratorMetrics {
public:
    void onStepStart() {
        stepCounter_.startStep();
    }

    void onStepFinish() {
        stepCounter_.finishStep();
    }

    void reset() {
        stepCounter_.reset();
    }

    [[nodiscard]] const RateCounter& stepCounter() const {
        return stepCounter_;
    }

private:
    RateCounter stepCounter_;
};
