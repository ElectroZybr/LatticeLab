#include <algorithm>
#include <format>
#include <string>

#include <Lattice/Tools/TableFormatter.hpp>

namespace Lattice {
namespace {

size_t codepointBytes(unsigned char value) noexcept {
    if ((value & 0x80) == 0) return 1;
    if ((value & 0xE0) == 0xC0) return 2;
    if ((value & 0xF0) == 0xE0) return 3;
    if ((value & 0xF8) == 0xF0) return 4;
    return 1;
}

size_t textWidth(std::string_view text) noexcept {
    size_t width = 0;
    for (size_t index = 0; index < text.size();) {
        index += std::min(codepointBytes(static_cast<unsigned char>(text[index])), text.size() - index);
        ++width;
    }
    return width;
}

std::string clipText(std::string_view text, size_t width) {
    if (textWidth(text) <= width)
        return std::string(text);
    if (width == 0)
        return {};
    if (width == 1)
        return "…";

    const size_t target = width - 1;
    size_t bytes = 0;
    size_t count = 0;
    while (bytes < text.size() && count < target) {
        bytes += std::min(
            codepointBytes(static_cast<unsigned char>(text[bytes])),
            text.size() - bytes
        );
        ++count;
    }
    return std::string(text.substr(0, bytes)) + "…";
}

void appendRepeated(TextFormatter& text, std::string_view glyph, size_t count, TextStyle style) {
    for (size_t index = 0; index < count; ++index)
        text.append(glyph, style);
}

void appendAligned(
    TextFormatter& output,
    std::string_view value,
    size_t width,
    TableFormatter::Align alignment,
    TextStyle style
) {
    const std::string clipped = clipText(value, width);
    const size_t valueWidth = textWidth(clipped);
    const size_t remaining = width - valueWidth;
    size_t left = 0;

    if (alignment == TableFormatter::Align::Right)
        left = remaining;
    else if (alignment == TableFormatter::Align::Center)
        left = remaining / 2;

    output.append(std::string(left, ' '), style);
    output.append(clipped, style);
    output.append(std::string(remaining - left, ' '), style);
}

}

const TableFormatter::CellFormats::Entry* TableFormatter::CellFormats::find(
    TableType type
) const noexcept {
    const auto found = entries_.find(std::string(type.name));
    return found == entries_.end() ? nullptr : &found->second;
}

TableFormatter::TableFormatter() {
    formats_.add<std::string>([](const std::string& value) { return value; });
    formats_.add<bool>([](bool value) { return value ? "true" : "false"; });

    formats_.add<int8_t>([](int8_t value) { return std::to_string(value); }, Align::Right);
    formats_.add<uint8_t>([](uint8_t value) { return std::to_string(value); }, Align::Right);
    formats_.add<int16_t>([](int16_t value) { return std::to_string(value); }, Align::Right);
    formats_.add<uint16_t>([](uint16_t value) { return std::to_string(value); }, Align::Right);
    formats_.add<int32_t>([](int32_t value) { return std::to_string(value); }, Align::Right);
    formats_.add<uint32_t>([](uint32_t value) { return std::to_string(value); }, Align::Right);
    formats_.add<int64_t>([](int64_t value) { return std::to_string(value); }, Align::Right);
    formats_.add<uint64_t>([](uint64_t value) { return std::to_string(value); }, Align::Right);
    formats_.add<float>([](float value) { return std::format("{}", value); }, Align::Right);
    formats_.add<double>([](double value) { return std::format("{}", value); }, Align::Right);
}

TableFormatter::View::View(
    const Table& table,
    const CellFormats& formats,
    Desc desc
) : table_(&table), desc_(std::move(desc)) {
    shownRows_ = std::min(table.rows(), desc_.maxRows);
    truncated_ = shownRows_ < table.rows();
    columns_.reserve(table.columns());

    for (size_t index = 0; index < table.columns(); ++index) {
        const ColumnView column = table.column(index);
        const auto* format = formats.find(column.type());
        size_t width = textWidth(column.name());

        for (size_t row = 0; row < shownRows_; ++row) {
            const std::string value = format
                ? format->format(column.element(row))
                : "<?>";
            width = std::max(width, textWidth(value));
        }

        width = std::min(width, desc_.maxColumnWidth);
        columns_.push_back({column, format, width});
    }
}

size_t TableFormatter::View::lines() const noexcept {
    if (columns_.empty())
        return 1;

    const bool outer = hasRule(desc_.style.rules, Rules::Outer);
    const bool header = hasRule(desc_.style.rules, Rules::Header);
    const bool rows = hasRule(desc_.style.rules, Rules::Rows);
    const size_t displayedRows = shownRows_;

    return (outer ? 1 : 0) + 1 + (header ? 1 : 0) + displayedRows +
        (rows && displayedRows > 1 ? displayedRows - 1 : 0) +
        (outer ? 1 : 0) + (truncated_ ? 1 : 0);
}

TextFormatter TableFormatter::View::Iterator::operator*() const {
    return view_->renderLine(line_);
}

TextFormatter TableFormatter::View::renderLine(size_t line) const {
    if (columns_.empty()) {
        TextFormatter result;
        result.append("(no columns)", desc_.style.cell);
        return result;
    }

    const bool outer = hasRule(desc_.style.rules, Rules::Outer);
    const bool header = hasRule(desc_.style.rules, Rules::Header);
    const bool rowRules = hasRule(desc_.style.rules, Rules::Rows);

    if (outer) {
        if (line == 0)
            return renderHorizontal(true, false);
        --line;
    }

    if (line == 0)
        return renderHeader();
    --line;

    if (header) {
        if (line == 0)
            return renderHorizontal(false, false);
        --line;
    }

    const size_t displayedRows = shownRows_;
    if (displayedRows != 0) {
        if (rowRules) {
            const size_t bodyLines = displayedRows * 2 - 1;
            if (line < bodyLines) {
                if (line % 2 != 0)
                    return renderHorizontal(false, false);
                const size_t row = line / 2;
                return renderRow(row);
            }
            line -= bodyLines;
        } else if (line < displayedRows) {
            return renderRow(line);
        } else {
            line -= displayedRows;
        }
    }

    if (outer) {
        if (line == 0)
            return renderHorizontal(false, true);
        --line;
    }

    if (truncated_ && line == 0)
        return renderTruncation();

    throw Exception<TableFormatter::View>("Line is out of range");
}

TextFormatter TableFormatter::View::renderHorizontal(bool top, bool bottom) const {
    const auto& style = desc_.style;
    const auto& glyphs = style.borders;
    const bool outer = hasRule(style.rules, Rules::Outer);
    const bool columnRules = hasRule(style.rules, Rules::Columns);
    TextFormatter result;

    const std::string_view left = top ? glyphs.topLeft : bottom ? glyphs.bottomLeft : glyphs.middleLeft;
    const std::string_view join = top ? glyphs.topJoin : bottom ? glyphs.bottomJoin : glyphs.middleJoin;
    const std::string_view right = top ? glyphs.topRight : bottom ? glyphs.bottomRight : glyphs.middleRight;

    if (outer)
        result.append(left, style.border);

    for (size_t index = 0; index < columns_.size(); ++index) {
        appendRepeated(
            result,
            glyphs.horizontal,
            style.paddingLeft + columns_[index].width + style.paddingRight,
            style.border
        );
        if (index + 1 < columns_.size() && columnRules)
            result.append(join, style.border);
    }

    if (outer)
        result.append(right, style.border);
    return result;
}

TextFormatter TableFormatter::View::renderHeader() const {
    const auto& style = desc_.style;
    const bool outer = hasRule(style.rules, Rules::Outer);
    const bool columnRules = hasRule(style.rules, Rules::Columns);
    TextFormatter result;

    if (outer)
        result.append(style.borders.vertical, style.border);

    for (size_t index = 0; index < columns_.size(); ++index) {
        result.append(std::string(style.paddingLeft, ' '), style.header);
        appendAligned(
            result,
            columns_[index].column.name(),
            columns_[index].width,
            Align::Left,
            style.header
        );
        result.append(std::string(style.paddingRight, ' '), style.header);

        if (index + 1 < columns_.size() && columnRules)
            result.append(style.borders.vertical, style.border);
    }

    if (outer)
        result.append(style.borders.vertical, style.border);
    return result;
}

TextFormatter TableFormatter::View::renderRow(size_t row) const {
    const auto& style = desc_.style;
    const bool outer = hasRule(style.rules, Rules::Outer);
    const bool columnRules = hasRule(style.rules, Rules::Columns);
    const TextStyle cellStyle = row % 2 == 0 ? style.cell : style.alternateCell;
    TextFormatter result;

    if (outer)
        result.append(style.borders.vertical, style.border);

    for (size_t index = 0; index < columns_.size(); ++index) {
        const auto& layout = columns_[index];
        const std::string value = layout.format
            ? layout.format->format(layout.column.element(row))
            : "<?>";
        const Align alignment = layout.format
            ? layout.format->alignment
            : Align::Left;

        result.append(std::string(style.paddingLeft, ' '), cellStyle);
        appendAligned(result, value, layout.width, alignment, cellStyle);
        result.append(std::string(style.paddingRight, ' '), cellStyle);

        if (index + 1 < columns_.size() && columnRules)
            result.append(style.borders.vertical, style.border);
    }

    if (outer)
        result.append(style.borders.vertical, style.border);
    return result;
}

TextFormatter TableFormatter::View::renderTruncation() const {
    TextFormatter result;
    result.append(
        std::format("... {} more rows", table_->rows() - shownRows_),
        desc_.style.truncation
    );
    return result;
}

}
