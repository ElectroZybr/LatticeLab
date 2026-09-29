#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include <Lattice/Tools/BmRunner/BenchTypes.hpp>

namespace Lattice {

struct StageContext {
    size_t n = 0;
    size_t samples = 9;
    size_t iterations = 1;

    Result& result;

    std::function<void()> prepare;
    std::function<void(size_t)> invoke;
    std::function<void(
        std::string_view,
        size_t,
        size_t,
        std::span<const Metric>
    )> progress;
};

class StageCapability {
public:
    virtual ~StageCapability() = default;

    virtual void begin(StageContext&) {}
    virtual void end(StageContext&) {}
    virtual void finish(StageContext&) {}
};

class StageDriver : public StageCapability {
public:
    virtual void run(
        StageContext& context,
        std::span<StageCapability*> capabilities
    ) = 0;
};

}