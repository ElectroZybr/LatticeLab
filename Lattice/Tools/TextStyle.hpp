#pragma once

#include <charconv>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Kernel/Value.hpp>

namespace Lattice {

struct TextColor {
    enum class Kind : uint8_t {
        Default,
        Rgb
    };

    Kind kind = Kind::Default;
    uint32_t value = 0;

    static constexpr TextColor rgb(uint32_t value) noexcept {
        return {Kind::Rgb, value & 0x00ffffffu};
    }

    bool operator==(const TextColor&) const = default;
};


struct TextStyle {
    enum Token : uint32_t {
        None = 0,
        Bold,
        Dim
    };

    enum Flag : uint8_t {
        NoFlags = 0,
        BoldFlag = 1u << 0,
        DimFlag = 1u << 1
    };

    uint8_t flags = NoFlags;
    TextColor color;

    constexpr TextStyle(Token token = None) noexcept {
        if (token == Bold)
            flags = BoldFlag;
        else if (token == Dim)
            flags = DimFlag;
    }

    static constexpr TextStyle rgb(uint32_t value) noexcept {
        TextStyle style;
        style.color = TextColor::rgb(value);
        return style;
    }

    static constexpr TextStyle rgb(uint8_t red, uint8_t green, uint8_t blue) noexcept {
        return rgb(
            (static_cast<uint32_t>(red) << 16) |
            (static_cast<uint32_t>(green) << 8) |
            static_cast<uint32_t>(blue)
        );
    }

    bool operator==(const TextStyle&) const = default;

};


constexpr TextStyle operator|(TextStyle current, TextStyle added) noexcept {
    current.flags |= added.flags;
    if (added.color.kind != TextColor::Kind::Default)
        current.color = added.color;
    return current;
}

constexpr TextStyle operator|(TextStyle::Token current, TextStyle::Token added) noexcept {
    return TextStyle{current} | TextStyle{added};
}

constexpr TextStyle operator|(TextStyle current, TextStyle::Token added) noexcept {
    return current | TextStyle{added};
}

constexpr TextStyle operator|(TextStyle::Token current, TextStyle added) noexcept {
    return TextStyle{current} | added;
}


constexpr bool hasStyle(TextStyle value, TextStyle expected) noexcept {
    const bool flags = expected.flags != TextStyle::NoFlags &&
        (value.flags & expected.flags) == expected.flags;
    const bool color = expected.color.kind != TextColor::Kind::Default &&
        value.color == expected.color;
    return flags || color;
}


inline std::string textStyleString(TextStyle style) {
    std::string result;

    if (style.color.kind == TextColor::Kind::Rgb)
        result = std::format("#{:06x}", style.color.value);

    const auto appendFlag = [&](std::string_view flag) {
        if (!result.empty()) result += ' ';
        result += flag;
    };
    if ((style.flags & TextStyle::BoldFlag) != 0) appendFlag("bold");
    if ((style.flags & TextStyle::DimFlag) != 0) appendFlag("dim");

    return result.empty() ? "default" : result;
}


inline TextStyle parseTextStyle(std::string_view source) {
    TextStyle result;
    bool sawDefault = false;

    while (!source.empty()) {
        const size_t separator = source.find_first_of(" \t\r\n");
        const std::string_view token = source.substr(0, separator);
        if (separator == std::string_view::npos)
            source = {};
        else {
            source.remove_prefix(separator + 1);
            const size_t content = source.find_first_not_of(" \t\r\n");
            source = content == std::string_view::npos ? std::string_view{} : source.substr(content);
        }

        if (token.empty())
            continue;
        if (token == "default") {
            sawDefault = true;
            continue;
        }
        if (token == "bold") {
            result = result | TextStyle::Bold;
            continue;
        }
        if (token == "dim") {
            result = result | TextStyle::Dim;
            continue;
        }

        TextStyle color;
        if (token.size() == 7 && token.front() == '#') {
            uint32_t rgb = 0;
            const auto [end, error] = std::from_chars(
                token.data() + 1,
                token.data() + token.size(),
                rgb,
                16
            );
            if (error != std::errc{} || end != token.data() + token.size())
                throw Exception<TextStyle>("Invalid RGB color '{}'", token);
            color = TextStyle::rgb(rgb);
        } else {
            throw Exception<TextStyle>("Unknown style token '{}'", token);
        }

        if (result.color.kind != TextColor::Kind::Default)
            throw Exception<TextStyle>("Style contains more than one color");
        result = result | color;
    }

    if (sawDefault && result != TextStyle{})
        throw Exception<TextStyle>("'default' cannot be combined with another style");
    return result;
}


inline bool parseTextStyleTag(std::string_view tag, TextStyle& style) {
    if (tag == "b") style = TextStyle::Bold;
    else if (tag == "d") style = TextStyle::Dim;
    else if (tag.size() == 7 && tag.front() == '#') style = parseTextStyle(tag);
    else return false;
    return true;
}


template<>
struct ParamAdapter<TextStyle> {
    static Value get(const TextStyle& style) {
        return Value{textStyleString(style)};
    }

    static void set(TextStyle& target, const Value& value) {
        target = parseTextStyle(value.get<std::string>());
    }
};

}
