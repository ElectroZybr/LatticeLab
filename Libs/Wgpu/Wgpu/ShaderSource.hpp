#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Wgpu {

/// WGSL embedded at build time (utils.bin2c, null-terminated):
///     constexpr unsigned char Walls[] = {
///     #include "Walls.wgsl.h"
///     };
/// The text of such a source.
template<size_t N>
std::string_view shaderText(const unsigned char (&source)[N]) {
    return {reinterpret_cast<const char*>(source), N - 1};
}

/// Value of `const NAME: u32 = <n>u;` in an embedded WGSL source, at compile time, or -1
/// when there is none. A static_assert against the C++ value keeps a shader and the code
/// it mirrors (operation codes, record sizes) in step.
template<size_t N>
constexpr int64_t shaderConstant(const unsigned char (&source)[N], std::string_view name) {
    constexpr std::string_view keyword = "const ";
    auto at = [&](size_t i, std::string_view text) {
        if (i + text.size() > N)
            return false;
        for (size_t k = 0; k < text.size(); ++k)
            if (source[i + k] != static_cast<unsigned char>(text[k]))
                return false;
        return true;
    };
    for (size_t i = 0; i + keyword.size() < N; ++i) {
        if (!at(i, keyword) || !at(i + keyword.size(), name) || source[i + keyword.size() + name.size()] != ':')
            continue;
        size_t j = i + keyword.size() + name.size();
        while (j < N && source[j] != '=')
            ++j;
        while (j < N && (source[j] == '=' || source[j] == ' '))
            ++j;
        int64_t value = -1;
        for (; j < N && source[j] >= '0' && source[j] <= '9'; ++j)
            value = (value < 0 ? 0 : value * 10) + (source[j] - '0');
        return value;
    }
    return -1;
}

}
