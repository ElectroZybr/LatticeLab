#include "BigO.hpp"

#include <array>
#include <cmath>
#include <limits>

namespace Lattice::Benchmarks {

namespace {

struct Model {
    Complexity complexity;
    double (*function)(double);
};

constexpr std::array<Model, 6> models{{
            {Complexity::Constant,  [](double) { return 1.0; }},
            {Complexity::LogN,      [](double n) { return std::log(n); }},
            {Complexity::Linear,    [](double n) { return n; }},
            {Complexity::NLogN,     [](double n) { return n * std::log(n); }},
            {Complexity::Quadratic, [](double n) { return n * n; }},
            {Complexity::Cubic,     [](double n) { return n * n * n; }}
}};

BigOResult fit(
    std::span<const ComplexityPoint> points,
    const Model& model
) {
    double logCoefficient = 0;

    for (const ComplexityPoint& point : points) {
        const double basis = model.function(point.x);

        if (
            point.x <= 0.0 ||
            point.y <= 0.0 ||
            basis <= 0.0 ||
            !std::isfinite(point.x) ||
            !std::isfinite(point.y) ||
            !std::isfinite(basis)
        ) {
            return {};
        }

        logCoefficient += std::log(point.y / basis);
    }

    logCoefficient /= static_cast<double>(points.size());

    const double coefficient = std::exp(logCoefficient);
    double squaredError = 0;

    for (const ComplexityPoint& point : points) {
        const double predicted =
            coefficient * model.function(point.x);

        const double error =
            std::log(point.y / predicted);

        squaredError += error * error;
    }

    const double rms =
        std::sqrt(
            squaredError /
            static_cast<double>(points.size())
        );

    return {
        .complexity = model.complexity,
        .coefficient = coefficient,
        .error = std::expm1(rms)
    };
}

}

BigOResult analyze(
    std::span<const ComplexityPoint> points
) {
    if (points.size() < 2)
        return {};

    BigOResult best;
    best.error = std::numeric_limits<double>::infinity();

    for (const Model& model : models) {
        const BigOResult result = fit(points, model);

        if (
            result.complexity != Complexity::Unknown &&
            result.error < best.error
        ) {
            best = result;
        }
    }

    if (!std::isfinite(best.error))
        return {};

    return best;
}

std::string_view complexityName(
    Complexity complexity
) noexcept {
    switch (complexity) {
        case Complexity::Constant:  return "O(1)";
        case Complexity::LogN:      return "O(log N)";
        case Complexity::Linear:    return "O(N)";
        case Complexity::NLogN:     return "O(N log N)";
        case Complexity::Quadratic: return "O(N^2)";
        case Complexity::Cubic:     return "O(N^3)";
        case Complexity::Unknown:   return "Unknown";
    }

    return "Unknown";
}

}