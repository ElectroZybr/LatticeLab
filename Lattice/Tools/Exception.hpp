#pragma once

#include <format>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <Lattice/Tools/TypeName.hpp>

namespace Lattice {

class ExceptionBase : public std::runtime_error {
public:
    ExceptionBase(
        std::string_view tag,
        std::string message,
        std::source_location source = std::source_location::current()
    )
        : std::runtime_error(std::move(message)),
          tag_(tag),
          source_(source) {}

    std::string_view tag() const noexcept {
        return tag_;
    }

    const std::source_location& source() const noexcept {
        return source_;
    }

private:
    std::string tag_;
    std::source_location source_;
};

template<typename... TArgs>
class ExceptionFormat {
public:
    template<typename TFormat>
    consteval ExceptionFormat(
        const TFormat& format,
        std::source_location source = std::source_location::current()
    ) : format_(format), source_(source) {}

    const std::format_string<TArgs...>& format() const noexcept {
        return format_;
    }

    const std::source_location& source() const noexcept {
        return source_;
    }

private:
    std::format_string<TArgs...> format_;
    std::source_location source_;
};

template<typename Owner>
class Exception final : public ExceptionBase {
public:
    template<typename... TArgs>
    Exception(
        ExceptionFormat<std::type_identity_t<TArgs>...> format,
        TArgs&&... args
    )
        : ExceptionBase(
            typeName<Owner>(),
            std::format(format.format(), std::forward<TArgs>(args)...),
            format.source()
        ) {}
};

}
