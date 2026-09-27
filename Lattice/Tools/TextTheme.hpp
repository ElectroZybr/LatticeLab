#pragma once

#include <cstdint>
#include <deque>
#include <limits>
#include <string>
#include <string_view>

#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/NamedRegistry.hpp>
#include <Lattice/Tools/TextStyle.hpp>

namespace Lattice {

using TextStyleId = uint32_t;
inline constexpr TextStyleId InvalidTextStyleId = std::numeric_limits<TextStyleId>::max();


class TextTheme {
public:
    struct Entry {
        std::string name;
        TextStyle style;
    };

    TextStyleId add(std::string_view name, TextStyle style) {
        if (isReserved(name))
            throw Exception("TextTheme", "Style name '{}' is reserved", name);

        const TextStyleId id = static_cast<TextStyleId>(entries_.size());
        names_.add(name, id);
        entries_.push_back({std::string(name), style});
        return id;
    }

    TextStyleId find(std::string_view name) const {
        return names_.find(name);
    }

    const TextStyle* findStyle(std::string_view name) const {
        const TextStyleId id = find(name);
        return id == InvalidTextStyleId ? nullptr : &entries_[id].style;
    }

    const TextStyle& style(TextStyleId id) const {
        return entries_.at(id).style;
    }

    TextStyle& style(TextStyleId id) {
        return entries_.at(id).style;
    }

    const std::deque<Entry>& entries() const noexcept {
        return entries_;
    }

    std::deque<Entry>& entries() noexcept {
        return entries_;
    }

    static TextTheme defaults() {
        TextTheme theme;
        theme.add("light", TextStyle::rgb(0xE6EDF3));
        theme.add("h",   TextStyle::Bold | TextStyle::rgb(0x7aa2f7));
        theme.add("h2",  TextStyle::Bold | TextStyle::rgb(0x4f7cff));
        theme.add("a",   TextStyle::rgb(0xbb9af7));
        theme.add("a2",  TextStyle::rgb(0x17A7CC));
        theme.add("ok",  TextStyle::rgb(0x28D08A));
        theme.add("wrn", TextStyle::rgb(0xffff55));
        theme.add("mut", TextStyle::rgb(0xaaaaaa));
        theme.add("mut2",TextStyle::rgb(0x555555));
        theme.add("err", TextStyle::rgb(0xDA6A6A));
        return theme;
    }

    static const TextTheme& system() {
        static const TextTheme theme = defaults();
        return theme;
    }

private:
    static bool isReserved(std::string_view name) {
        if (name.empty() || name.front() == '#' || name.front() == '/')
            return true;

        TextStyle ignored;
        return parseTextStyleTag(name, ignored);
    }

    NamedRegistry<TextStyleId> names_;
    std::deque<Entry> entries_;
};

}
