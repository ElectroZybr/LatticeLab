#pragma once

#include <algorithm>
#include <cstdint>

#include <Lattice/Tools/TextMarkup.hpp>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <cctype>



namespace Lattice {


struct TextSpan {
    std::string text;
    TextStyle style = TextStyle::None;
};


class TextFormatter {
public:

    TextFormatter() = default;

    explicit TextFormatter(std::string_view text, const TextTheme* theme = nullptr) {
        parse(text, theme);
    }

    TextFormatter(std::string_view text, const TextTheme& theme) {
        parse(text, &theme);
    }


    template<typename... TArgs>
    static TextFormatter format(
        std::format_string<TArgs...> fmt,
        TArgs&&... args
    ) {
        return TextFormatter(
            std::format(
                fmt,
                std::forward<TArgs>(args)...
            )
        );
    }


    template<typename... TArgs>
    static TextFormatter format(
        const TextTheme& theme,
        std::format_string<TArgs...> fmt,
        TArgs&&... args
    ) {
        return TextFormatter(
            std::format(fmt, std::forward<TArgs>(args)...),
            theme
        );
    }


    static TextFormatter format(const TextFormatter& text) {
        return text;
    }


    TextFormatter& operator+=(const TextFormatter& other) {
        append(other);
        return *this;
    }


    friend TextFormatter operator+(TextFormatter lhs, const TextFormatter& rhs) {
        lhs += rhs;
        return lhs;
    }


    TextFormatter& operator+=(std::string_view text) {
        append(text);
        return *this;
    }


    friend TextFormatter operator+(TextFormatter lhs, std::string_view rhs) {
        lhs += rhs;
        return lhs;
    }


    friend TextFormatter operator+(std::string_view lhs, TextFormatter rhs) {
        TextFormatter result(lhs);
        result += rhs;
        return result;
    }


    void parse(std::string_view text, const TextTheme* theme = nullptr) {
        spans_.clear();
        diagnostics_.clear();
        if (!theme)
            theme = &TextTheme::system();

        TextMarkup::parse(
            text,
            *theme,
            [this, theme](std::string_view content, const TextMarkup::StyleChain& chain) {
                append(content, TextMarkup::resolve(chain, *theme));
            },
            [this](TextDiagnostic diagnostic) {
                diagnostics_.push_back(std::move(diagnostic));
            },
            "TextFormatter"
        );
    }


    void append(std::string_view text, TextStyle style = TextStyle::None) {
        if (text.empty())
            return;

        if (!spans_.empty() && spans_.back().style == style) {
            spans_.back().text += text;
            return;
        }

        spans_.push_back({
            .text = std::string(text),
            .style = style
        });
    }

    void append(const TextFormatter& text) {
        for (const auto& span : text.spans_)
            append(span.text, span.style);
        appendDiagnostics(text.diagnostics_);
    }


    void append(
        const TextFormatter& text,
        TextStyle style
    ) {
        for (const auto& span : text.spans_) {
            append(
                span.text,
                style | span.style
            );
        }
        appendDiagnostics(text.diagnostics_);
    }

    void appendDiagnostics(std::span<const TextDiagnostic> diagnostics) {
        diagnostics_.insert(diagnostics_.end(), diagnostics.begin(), diagnostics.end());
    }

    std::span<const TextDiagnostic> diagnostics() const noexcept {
        return diagnostics_;
    }


    std::string plain() const {
        std::string result;

        for (const auto& span : spans_)
            result += span.text;

        return result;
    }


    std::string markup() const {
        std::string result;

        for (const auto& span : spans_) {
            size_t count = 0;
            const auto appendTag = [&](std::string_view tag) {
                result += '<';
                result += tag;
                result += '>';
                ++count;
            };

            if (hasStyle(span.style, TextStyle::Bold)) appendTag("b");
            if (hasStyle(span.style, TextStyle::Dim)) appendTag("d");

            if (span.style.color.kind == TextColor::Kind::Rgb)
                appendTag(std::format("#{:06x}", span.style.color.value));

            result += span.text;

            if (count != 0) {
                result += '<';
                result.append(count, '/');
                result += '>';
            }
        }

        return result;
    }


    std::string render() const {
        std::string result;

        for (const auto& span : spans_) {

            if (span.style != TextStyle::None)
                result += ansi(span.style);

            result += span.text;
            result += "\033[0m";
        }

        return result;
    }


    size_t length() const noexcept {
        size_t result = 0;

        for (const auto& span : spans_)
            result += span.text.size();

        return result;
    }


    size_t lines() const noexcept {
        if (spans_.empty())
            return 0;

        size_t result = 1;

        for (const auto& span : spans_) {
            result += std::count(
                span.text.begin(),
                span.text.end(),
                '\n'
            );
        }

        return result;
    }


    TextFormatter wrap(
        size_t width,
        size_t continuationIndent = 0
    ) const {

        if (width == 0)
            throw Exception(
                "TextFormatter",
                "Wrap width cannot be zero"
            );

        if (continuationIndent >= width)
            throw Exception(
                "TextFormatter",
                "Continuation indent must be less than wrap width"
            );

        TextFormatter result;

        size_t lineLength = 0;

        auto newWrappedLine = [&]() {
            result.append("\n");

            if (continuationIndent > 0) {
                result.append(
                    std::string(
                        continuationIndent,
                        ' '
                    ),
                    TextStyle::None
                );
            }

            lineLength = continuationIndent;
        };


        auto newExplicitLine = [&]() {
            result.append("\n");
            lineLength = 0;
        };


        for (const auto& span : spans_) {

            size_t position = 0;

            while (position < span.text.size()) {

                const size_t newline =
                    span.text.find('\n', position);

                const size_t lineEnd =
                    newline == std::string::npos
                        ? span.text.size()
                        : newline;

                std::string_view line =
                    std::string_view(span.text).substr(
                        position,
                        lineEnd - position
                    );

                while (!line.empty()) {

                    while (
                        !line.empty() &&
                        isSpace(line.front())
                    ) {
                        if (line.front() == ' ')
                            ++lineLength;

                        line.remove_prefix(1);
                    }

                    if (line.empty())
                        break;

                    size_t wordEnd = 0;

                    while (
                        wordEnd < line.size() &&
                        !isSpace(line[wordEnd])
                    ) {
                        ++wordEnd;
                    }

                    const std::string_view word =
                        line.substr(0, wordEnd);

                    const size_t wordLength =
                        utf8Length(word);

                    if (
                        lineLength != 0 &&
                        lineLength + 1 + wordLength > width
                    ) {
                        newWrappedLine();
                    }

                    if (
                        wordLength >
                        width - lineLength
                    ) {

                        size_t consumed = 0;

                        while (consumed < word.size()) {

                            size_t bytes = 0;
                            size_t chars = 0;

                            while (
                                consumed + bytes < word.size() &&
                                lineLength + chars < width
                            ) {

                                const unsigned char c =
                                    static_cast<unsigned char>(
                                        word[consumed + bytes]
                                    );

                                const size_t charBytes =
                                    (c & 0x80) == 0 ? 1 :
                                    (c & 0xE0) == 0xC0 ? 2 :
                                    (c & 0xF0) == 0xE0 ? 3 :
                                    (c & 0xF8) == 0xF0 ? 4 :
                                    1;

                                bytes += charBytes;
                                ++chars;
                            }

                            if (
                                lineLength != 0 &&
                                consumed == 0 &&
                                chars == 0
                            ) {
                                newWrappedLine();
                                continue;
                            }

                            result.append(
                                word.substr(
                                    consumed,
                                    bytes
                                ),
                                span.style
                            );

                            consumed += bytes;
                            lineLength += chars;

                            if (consumed < word.size())
                                newWrappedLine();
                        }

                    } else {

                        if (lineLength != 0) {
                            result.append(
                                " ",
                                TextStyle::None
                            );

                            ++lineLength;
                        }

                        result.append(
                            word,
                            span.style
                        );

                        lineLength += wordLength;
                    }

                    line.remove_prefix(wordEnd);
                }

                if (newline != std::string::npos) {
                    newExplicitLine();
                    position = newline + 1;
                } else {
                    position = span.text.size();
                }
            }
        }

        result.appendDiagnostics(diagnostics_);
        return result;
    }


private:
    static std::string ansi(
        TextStyle style
    ) {

        std::string result;

        if (hasStyle(style, TextStyle::Bold))
            result += "\033[1m";

        if (hasStyle(style, TextStyle::Dim))
            result += "\033[2m";

        if (style.color.kind == TextColor::Kind::Rgb) {
            const uint32_t rgb = style.color.value;
            result += std::format(
                "\033[38;2;{};{};{}m",
                (rgb >> 16) & 0xff,
                (rgb >> 8) & 0xff,
                rgb & 0xff
            );
        }

        return result;
    }


    static size_t utf8Length(
        std::string_view text
    ) {

        size_t length = 0;

        for (size_t i = 0; i < text.size();) {

            const unsigned char c =
                static_cast<unsigned char>(text[i]);

            if ((c & 0x80) == 0)
                i += 1;

            else if ((c & 0xE0) == 0xC0)
                i += 2;

            else if ((c & 0xF0) == 0xE0)
                i += 3;

            else if ((c & 0xF8) == 0xF0)
                i += 4;

            else
                i += 1;

            ++length;
        }

        return length;
    }


    static bool isSpace(char c) {
        return std::isspace(
            static_cast<unsigned char>(c)
        );
    }


    std::vector<TextSpan> spans_;
    std::vector<TextDiagnostic> diagnostics_;
};


} // namespace Lattice


using Lattice::TextFormatter;
using Lattice::TextStyle;
