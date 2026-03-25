#include "DebugView.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <vector>
#include "imgui.h"

DebugView::DebugView(std::string_view title, std::initializer_list<DebugEntry> entries) : title(title) {
    data.reserve(entries.size());
    indicesByLabel.reserve(entries.size());

    for (const auto& e : entries) {
        const std::size_t index = data.size();
        Storage storage = (e.type == DebugDisplayType::Series)
            ? Storage{ std::deque<float>{} }
            : Storage{ std::any{} };

        data.emplace_back(DebugData{ e, std::move(storage) });
        indicesByLabel.emplace(e.label, index);
    }
}

void DebugView::add_data(std::string_view label, std::any value) {
    auto it = indicesByLabel.find(std::string(label));
    if (it == indicesByLabel.end()) return;

    auto& d = data[it->second];
    if (d.entry.type == DebugDisplayType::Series) {
        auto& dq = std::get<std::deque<float>>(d.storage);
        if (dq.size() >= HISTORY_SIZE) dq.pop_front();
        dq.emplace_back(std::any_cast<float>(value));
    }
    else {
        d.storage = std::move(value);
    }
}

void DebugView::draw(float uiScale) {
    for (auto& d : data) {
        const std::string& label = d.entry.label;
        if (d.entry.type == DebugDisplayType::Value) {
            const auto& a = std::get<std::any>(d.storage);
            d.entry.draw(label, a, d.entry.precision);
        }
        else {
            auto& dq = std::get<std::deque<float>>(d.storage);
            const int precision = std::clamp(d.entry.precision, 0, 6);
            ImGui::TextDisabled("%s", (label + ": ").data());

            if (!dq.empty()) {
                ImGui::SameLine();
                ImGui::Text("%s", std::format("{:.{}f}", dq.back(), precision).data());
            }

            if (dq.size() >= 2) {
                const auto [minVal, maxVal] = std::ranges::minmax(dq);
                const float range = (maxVal - minVal) < 1e-6f ? 1.f : (maxVal - minVal);

                ImGui::TextDisabled("%s", std::format("{:.{}f}", maxVal, precision).data());

                std::vector<float> buf(dq.begin(), dq.end());
                ImGui::PlotLines(
                    label.data(), buf.data(), static_cast<int>(buf.size()), 0, nullptr,
                    minVal - range * 0.1f, maxVal + range * 0.1f, ImVec2(-1, 60 * uiScale));

                ImGui::TextDisabled("%s", std::format("{:.{}f}", minVal, precision).data());
            }
        }
        ImGui::Separator();
    }
}
