#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace Lattice::Benchmarks {

/**
 @file BigO.hpp
 @brief Анализ асимптотического роста числовой зависимости.

 BigO подбирает модель роста для набора точек и возвращает
 наиболее подходящую сложность, коэффициент и ошибку аппроксимации.
*/

enum class Complexity : uint8_t {
    Constant,
    LogN,
    Linear,
    NLogN,
    Quadratic,
    Cubic,
    Unknown
};

struct ComplexityPoint {
    double x = 0;
    double y = 0;
};

struct BigOResult {
    Complexity complexity = Complexity::Unknown;
    double coefficient = 0;
    double error = 0;
};

BigOResult analyze(std::span<const ComplexityPoint> points);
std::string_view complexityName(Complexity complexity) noexcept;

}