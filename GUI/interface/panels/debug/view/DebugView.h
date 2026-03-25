#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <initializer_list>
#include <variant>
#include <deque>
#include <any>

#include "../DebugEntry.h"

class DebugView {
public:
    using Storage = std::variant<std::any, std::deque<float>>;

    DebugView(std::string_view title, std::initializer_list<DebugEntry> entries);
    const char* getTitle() const { return title.data(); }
    void draw(float uiScale);

    void add_data(std::string_view label, std::any value);

    bool visible = true;
private:
    static constexpr int HISTORY_SIZE = 300;

    struct DebugData {
        DebugEntry entry;
        Storage storage;
    };

    std::string title;
    std::vector<DebugData> data;
    std::unordered_map<std::string, std::size_t> indicesByLabel;
};
