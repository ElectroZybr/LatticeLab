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

double constant(double) {
    return 1.0;
}

double logN(double x) {
    return x > 1.0
        ? std::log2(x)
        : 0.0;
}

double linear(double x) {
    return x;
}

double nLogN(double x) {
    return x > 1.0
        ? x * std::log2(x)
        : 0.0;
}

double quadratic(double x) {
    return x * x;
}

double cubic(double x) {
    return x * x * x;
}

constexpr std::array<Model, 6> models{{
    {Complexity::Constant, constant},
    {Complexity::LogN, logN},
    {Complexity::Linear, linear},
    {Complexity::NLogN, nLogN},
    {Complexity::Quadratic, quadratic},
    {Complexity::Cubic, cubic}
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