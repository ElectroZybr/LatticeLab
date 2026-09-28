#pragma once
#include <string_view>

namespace Lattice {

template<typename T>
constexpr std::string_view typeKey() {
    constexpr std::string_view p = __PRETTY_FUNCTION__;

#if defined(__clang__)
    constexpr std::string_view prefix = "std::string_view Lattice::typeKey() [T = ";
    constexpr auto start = prefix.size();
    constexpr auto end = p.find(']', start);
#elif defined(__GNUC__)
    constexpr std::string_view prefix = "constexpr std::string_view Lattice::typeKey() [with T = ";
    constexpr auto start = prefix.size();
    constexpr auto end = p.find(';', start);
#else
#error "typeName: unsupported compiler"
#endif

    constexpr auto full = p.substr(start, end - start);

    return full;
}

template<typename T>
constexpr std::string_view typeName() {
    constexpr auto full = typeKey<T>();
    constexpr auto templateStart = full.find('<');
    constexpr auto base = full.substr(0, templateStart);
    constexpr auto namespaceEnd = base.rfind("::");
    return namespaceEnd == std::string_view::npos
        ? base
        : base.substr(namespaceEnd + 2);
}

} // namespace Lattice
