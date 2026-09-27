#pragma once

#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Tools/TextTheme.hpp>

namespace Lattice {

struct TextDiagnostic {
    std::string owner;
    std::string message;
    std::string line;
    size_t lineNumber = 0;
    size_t column = 0;
};

namespace TextMarkup {

struct StyleOperation {
    TextStyle value;
    TextStyleId semantic = InvalidTextStyleId;

    bool isSemantic() const noexcept {
        return semantic != InvalidTextStyleId;
    }

    TextStyle resolve(const TextTheme& theme) const {
        return isSemantic() ? theme.style(semantic) : value;
    }

    bool operator==(const StyleOperation&) const = default;
};

using StyleChain = std::vector<StyleOperation>;

inline TextDiagnostic diagnostic(
    std::string_view owner,
    std::string_view message,
    std::string_view source,
    size_t position
) {
    const size_t previousLine = position == 0
        ? std::string_view::npos
        : source.rfind('\n', position - 1);
    const size_t lineStart = previousLine == std::string_view::npos ? 0 : previousLine + 1;
    const size_t lineEnd = source.find('\n', position);
    const std::string_view line = source.substr(
        lineStart,
        lineEnd == std::string_view::npos ? source.size() - lineStart : lineEnd - lineStart
    );
    const size_t lineNumber = 1 + static_cast<size_t>(std::count(
        source.begin(), source.begin() + position, '\n'
    ));
    const size_t column = position - lineStart + 1;

    return {
        .owner = std::string(owner),
        .message = std::string(message),
        .line = std::string(line),
        .lineNumber = lineNumber,
        .column = column
    };
}

inline bool builtin(std::string_view tag, TextStyle& style) {
    return parseTextStyleTag(tag, style);
}

inline bool closeTag(std::string_view tag) noexcept {
    if (tag.empty())
        return false;
    for (const char value : tag) {
        if (value != '/')
            return false;
    }
    return true;
}

inline bool resolve(
    std::string_view tag,
    const TextTheme& theme,
    StyleOperation& operation
) {
    if (builtin(tag, operation.value))
        return true;

    operation.semantic = theme.find(tag);
    return operation.isSemantic();
}

template<typename Append, typename Diagnose>
void parse(
    std::string_view source,
    const TextTheme& theme,
    Append&& append,
    Diagnose&& diagnose,
    std::string_view owner = "TextMarkup"
) {
    struct OpenTag {
        std::string_view name;
        size_t position;
    };

    StyleChain stack;
    std::vector<OpenTag> openTags;
    size_t position = 0;
    size_t textStart = 0;

    while (position < source.size()) {
        if (source[position] != '<') {
            ++position;
            continue;
        }

        const size_t end = source.find('>', position);
        if (end == std::string_view::npos) {
            ++position;
            continue;
        }

        const std::string_view tag = source.substr(position + 1, end - position - 1);
        if (closeTag(tag)) {
            if (tag.size() > stack.size()) {
                diagnose(diagnostic(
                    owner,
                    "closing tag has no matching opening tag",
                    source,
                    position
                ));
                position = end + 1;
                continue;
            }

            append(source.substr(textStart, position - textStart), stack);
            stack.resize(stack.size() - tag.size());
            openTags.resize(openTags.size() - tag.size());
        } else {
            StyleOperation operation;
            if (!resolve(tag, theme, operation)) {
                ++position;
                continue;
            }

            append(source.substr(textStart, position - textStart), stack);
            stack.push_back(operation);
            openTags.push_back({tag, position});
        }

        position = end + 1;
        textStart = position;
    }

    append(source.substr(textStart), stack);
    if (!openTags.empty()) {
        const OpenTag& first = openTags.front();
        const std::string message = openTags.size() == 1
            ? std::format("unclosed style tag '<{}>'", first.name)
            : std::format(
                "{} unclosed style tags, starting with '<{}>'",
                openTags.size(),
                first.name
            );
        diagnose(diagnostic(owner, message, source, first.position));
    }
}

inline TextStyle resolve(const StyleChain& chain, const TextTheme& theme) {
    TextStyle style;
    for (const StyleOperation& operation : chain)
        style = style | operation.resolve(theme);
    return style;
}

}

}
