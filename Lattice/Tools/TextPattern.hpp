#pragma once

#include <string>
#include <span>
#include <string_view>
#include <vector>

#include <Lattice/Tools/TextFormatter.hpp>
#include <Lattice/Tools/TextMarkup.hpp>

namespace Lattice {

/**
 * Parsed text markup for strings which are rendered repeatedly.
 *
 * Built-in styles are stored directly. Semantic tags are resolved to compact
 * TextStyleId values once, while their current value is read from the theme on
 * every format() call. The referenced theme must outlive the pattern.
 */
 
class TextPattern {
public:
    explicit TextPattern(
        std::string_view source,
        const TextTheme& theme = TextTheme::system()
    ) : theme_(&theme) {
        parse(source);
    }

    void parse(std::string_view source) {
        spans_.clear();
        diagnostics_.clear();
        TextMarkup::parse(
            source,
            *theme_,
            [this](std::string_view text, const TextMarkup::StyleChain& style) {
                append(text, style);
            },
            [this](TextDiagnostic diagnostic) {
                diagnostics_.push_back(std::move(diagnostic));
            },
            "TextPattern"
        );
    }

    TextFormatter format() const {
        TextFormatter result;
        for (const Span& span : spans_)
            result.append(span.text, TextMarkup::resolve(span.style, *theme_));
        result.appendDiagnostics(diagnostics_);
        return result;
    }

    std::string plain() const {
        std::string result;
        for (const Span& span : spans_)
            result += span.text;
        return result;
    }

    std::span<const TextDiagnostic> diagnostics() const noexcept {
        return diagnostics_;
    }

private:
    struct Span {
        std::string text;
        TextMarkup::StyleChain style;
    };

    void append(std::string_view text, const TextMarkup::StyleChain& style) {
        if (text.empty())
            return;
        if (!spans_.empty() && spans_.back().style == style) {
            spans_.back().text += text;
            return;
        }
        spans_.push_back({std::string(text), style});
    }

    const TextTheme* theme_;
    std::vector<Span> spans_;
    std::vector<TextDiagnostic> diagnostics_;
};

}
