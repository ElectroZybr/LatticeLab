#include "DebugView.h"

#include <cmath>
#include <format>
#include <algorithm>
#include <vector>

#include "imgui.h"

DebugView::DebugView(std::string_view title, std::initializer_list<DebugEntry> entries): title(title) {
    for (const auto& e : entries) {
        if (e.type == DebugDisplayType::Series)
            data.emplace(e.label, DebugData{ e, std::deque<float>{} });
        else
            data.emplace(e.label, DebugData{ e, 0.f });
    }
}

void DebugView::add_data(std::string_view label, float value) {
    auto it = data.find(label.data());
    if (it == data.end()) return;
    auto& d = it->second;
    if (d.entry.type == DebugDisplayType::Series) {
        auto& deque = std::get<std::deque<float>>(d.history);
        if (deque.size() >= HISTORY_SIZE) deque.pop_front();
        deque.emplace_back(value);
    }
    else {
        d.history = value;
    }
}

void DebugView::draw(float uiScale) {
    for (auto& [label, d] : data) {
        if (d.entry.type == DebugDisplayType::Value) {
            const float val = std::get<float>(d.history);
            const std::string valStr = (val == std::floor(val))
                ? std::format("{:.0f}", val)
                : (label.find("(мс)") != std::string::npos
                    ? std::format("{:.4f}", val)
                    : std::format("{:f}", val));
            ImGui::TextDisabled("%s", label.data());
            ImGui::SameLine();
            ImGui::Text("%s", valStr.data());

        }
        else {
            auto& deque = std::get<std::deque<float>>(d.history);

            const bool isSeries  = !deque.empty();
            const auto* series   = isSeries ? &deque : nullptr;
            ImGui::TextDisabled("%s", (label + ": ").data());
            if (!deque.empty()) {
                ImGui::SameLine();
                ImGui::Text("%s", std::format("{:f}", deque.back()).data());
            }

            if (deque.size() >= 2) {
                const float minVal = *std::ranges::min_element(deque);
                const float maxVal = *std::ranges::max_element(deque);
                const float range  = (maxVal - minVal) < 1e-6f ? 1.f : (maxVal - minVal);

                ImGui::TextDisabled("%s", std::format("{:f}", maxVal).data());

                std::vector<float> buf(deque.begin(), deque.end());
                ImGui::PlotLines(label.data(), buf.data(), buf.size(), 0, nullptr,
                    minVal - range * 0.1f, maxVal + range * 0.1f,
                    ImVec2(-1, 60 * uiScale));

                ImGui::TextDisabled("%s", std::format("{:f}", minVal).data());
            }
        }
        ImGui::Separator();
    }
}
