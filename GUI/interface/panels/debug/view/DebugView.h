#pragma once
#include <string>
#include <string_view>
#include <deque>
#include <unordered_map>
#include <variant>
#include <initializer_list>

#include "../DebugEntry.h"

class DebugView {
    static constexpr int HISTORY_SIZE = 300;

    struct DebugData {
        DebugEntry entry;
        std::variant<float, std::deque<float>> history;
    };

    std::string title;
    std::unordered_map<std::string, DebugData> data;
public:
    DebugView(std::string_view title, std::initializer_list<DebugEntry> entries);
    const char* getTitle() const { return title.data(); }
    void add_data(std::string_view label, float value);
    void draw(float uiScale);
};