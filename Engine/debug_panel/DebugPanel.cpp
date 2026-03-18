#include "DebugPanel.h"
#include "imgui.h"

DebugView* DebugPanel::addView(DebugView view) {
    views.reserve(4); // NOTE: Скорее всего вкладок будет не больше 4
    views.emplace_back(std::move(view));
    return &views.back();
}

void DebugPanel::draw(float uiScale, sf::Vector2u windowSize) {
    float target = visible ? 1.f : 0.f;
    float step = ImGui::GetIO().DeltaTime * 12.f;
    animProgress += (target - animProgress) * std::min(step, 1.f);

    if (animProgress < 0.01f) return;

    const float panelWidth  = 300.f * uiScale;
    const float panelHeight = static_cast<float>(windowSize.y);
    const float x = windowSize.x - panelWidth * animProgress;

    ImGui::SetNextWindowPos(ImVec2(x, 0));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::Begin("##DebugPanel", nullptr,
        ImGuiWindowFlags_NoMove    |
        ImGuiWindowFlags_NoResize  |
        ImGuiWindowFlags_NoCollapse|
        ImGuiWindowFlags_NoTitleBar
    );
    ImGui::PushFont(font);

    if (ImGui::BeginTabBar("##DebugTabs")) {
        for (auto& view : views) {
            if (ImGui::BeginTabItem(view.getTitle())) {
                view.draw(uiScale);
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::PopFont();
    ImGui::End();
}

void DebugPanel::loadFont(std::string_view path, float size) {
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Латиница
        0x0400, 0x04FF, // Кириллица
        0,
    };
    font = ImGui::GetIO().Fonts->AddFontFromFileTTF(path.data(), size, nullptr, ranges);
}