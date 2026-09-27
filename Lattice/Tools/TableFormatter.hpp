#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <Lattice/Kernel/TableAPI.hpp>
#include <Lattice/Tools/TextFormatter.hpp>


namespace Lattice {

class TableFormatter {
public:
    enum class Rules : uint8_t {
        None = 0,
        Outer = 1u << 0,
        Header = 1u << 1,
        Rows = 1u << 2,
        Columns = 1u << 3,
        All = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3)
    };

    friend constexpr Rules operator|(Rules left, Rules right) noexcept {
        return static_cast<Rules>(
            static_cast<uint8_t>(left) | static_cast<uint8_t>(right)
        );
    }

    friend constexpr Rules operator&(Rules left, Rules right) noexcept {
        return static_cast<Rules>(
            static_cast<uint8_t>(left) & static_cast<uint8_t>(right)
        );
    }

    enum class Align : uint8_t {
        Left,
        Center,
        Right
    };

    struct BorderGlyphs {
        std::string_view horizontal;
        std::string_view vertical;
        std::string_view topLeft;
        std::string_view topJoin;
        std::string_view topRight;
        std::string_view middleLeft;
        std::string_view middleJoin;
        std::string_view middleRight;
        std::string_view bottomLeft;
        std::string_view bottomJoin;
        std::string_view bottomRight;
    };

    struct Borders {
        inline static constexpr BorderGlyphs Ascii{
            "-", "|", "+", "+", "+", "+", "+", "+", "+", "+", "+"
        };
        inline static constexpr BorderGlyphs Sharp{
            "─", "│", "┌", "┬", "┐", "├", "┼", "┤", "└", "┴", "┘"
        };
        inline static constexpr BorderGlyphs Rounded{
            "─", "│", "╭", "┬", "╮", "├", "┼", "┤", "╰", "┴", "╯"
        };
        inline static constexpr BorderGlyphs Double{
            "═", "║", "╔", "╦", "╗", "╠", "╬", "╣", "╚", "╩", "╝"
        };
        inline static constexpr BorderGlyphs Spaces{
            " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " "
        };
    };

    struct Style {
        BorderGlyphs borders = Borders::Rounded;
        Rules rules = Rules::Outer | Rules::Header | Rules::Columns;
        TextStyle border = TextStyle::rgb(0x555555);
        TextStyle header = TextStyle::Bold | TextStyle::rgb(0x5555ff);
        TextStyle cell = TextStyle::None;
        TextStyle alternateCell = TextStyle::None;
        TextStyle truncation = TextStyle::rgb(0x555555);
        size_t paddingLeft = 1;
        size_t paddingRight = 1;
    };

    struct Desc {
        inline static constexpr size_t Unlimited = std::numeric_limits<size_t>::max();

        Style style;
        size_t maxRows = 32;
        size_t maxColumnWidth = 40;
    };

    class CellFormats {
    public:
        struct Entry {
            std::function<std::string(const void*)> format;
            Align alignment = Align::Left;
        };

        template<typename T, typename Formatter>
        void add(Formatter&& formatter, Align alignment = Align::Left) {
            using Value = std::remove_cvref_t<T>;
            using Function = std::decay_t<Formatter>;
            entries_.insert_or_assign(
                std::string(typeKey<Value>()),
                Entry{
                    .format = [function = Function(std::forward<Formatter>(formatter))](
                        const void* object
                    ) mutable {
                        return std::string(std::invoke(
                            function,
                            *static_cast<const Value*>(object)
                        ));
                    },
                    .alignment = alignment
                }
            );
        }

        const Entry* find(TableType type) const noexcept;

    private:
        std::unordered_map<std::string, Entry> entries_;
    };

    class View {
    public:
        class Iterator {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = TextFormatter;
            using difference_type = std::ptrdiff_t;

            Iterator() = default;
            Iterator(const View* view, size_t line) : view_(view), line_(line) {}

            TextFormatter operator*() const;
            Iterator& operator++() { ++line_; return *this; }
            void operator++(int) { ++line_; }

            friend bool operator==(const Iterator&, const Iterator&) = default;

        private:
            const View* view_ = nullptr;
            size_t line_ = 0;
        };

        Iterator begin() const noexcept { return {this, 0}; }
        Iterator end() const noexcept { return {this, lines()}; }
        size_t lines() const noexcept;

    private:
        friend class TableFormatter;
        friend class Iterator;

        struct ColumnLayout {
            ColumnView column;
            const CellFormats::Entry* format = nullptr;
            size_t width = 0;
        };

        View(const Table& table, const CellFormats& formats, Desc desc);

        TextFormatter renderLine(size_t line) const;
        TextFormatter renderHorizontal(bool top, bool bottom) const;
        TextFormatter renderHeader() const;
        TextFormatter renderRow(size_t row) const;
        TextFormatter renderTruncation() const;

        const Table* table_ = nullptr;
        Desc desc_;
        std::vector<ColumnLayout> columns_;
        size_t shownRows_ = 0;
        bool truncated_ = false;
    };

    TableFormatter();

    CellFormats& formats() noexcept { return formats_; }
    const CellFormats& formats() const noexcept { return formats_; }

    View view(const Table& table) const {
        return View{table, formats_, Desc{}};
    }

    View view(const Table& table, Desc desc) const {
        return View{table, formats_, std::move(desc)};
    }

private:
    static constexpr bool hasRule(Rules rules, Rules rule) noexcept {
        return (rules & rule) != Rules::None;
    }

    CellFormats formats_;
};

}
