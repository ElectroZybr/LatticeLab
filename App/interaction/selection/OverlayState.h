#pragma once

#include <cmath>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui_internal.h>

#include "Engine/math/Vec2.h"

struct OverlayState {
    bool boxVisible = false;
    bool lassoVisible = false;
    bool rulerVisible = false;

    Vec2i boxStart;
    Vec2i boxEnd;
    Vec2i rulerStart;
    Vec2i rulerEnd;
    std::string rulerLabel;

    std::vector<Vec2i> lassoPoints;

    void reset() {
        boxVisible = false;
        lassoVisible = false;
        rulerVisible = false;
        rulerLabel.clear();
        lassoPoints.clear();
    }

    void draw(sf::RenderTarget& target) const {
        if (!boxVisible && !lassoVisible && !rulerVisible) {
            return;
        }

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        constexpr ImU32 red = IM_COL32(255, 0, 0, 255);
        constexpr ImU32 light_red = IM_COL32(255, 90, 90, 255);

        if (boxVisible) {
            dl->AddRect(ImVec2(boxStart.x, boxStart.y), ImVec2(boxEnd.x, boxEnd.y), red);
        }

        if (lassoVisible && lassoPoints.size() >= 2) {
            for (size_t i = 0; i + 1 < lassoPoints.size(); ++i) {
                dl->AddLine(ImVec2(lassoPoints[i].x, lassoPoints[i].y), ImVec2(lassoPoints[i + 1].x, lassoPoints[i + 1].y), red);
            }
            dl->AddLine(ImVec2(lassoPoints.back().x, lassoPoints.back().y), ImVec2(lassoPoints.front().x, lassoPoints.front().y), red);
        }

        if (rulerVisible) {
            dl->AddLine(ImVec2(rulerStart.x, rulerStart.y), ImVec2(rulerEnd.x, rulerEnd.y), red, 1.5f);

            dl->AddCircleFilled(ImVec2(rulerStart.x, rulerStart.y), 4.f, light_red);
            dl->AddCircleFilled(ImVec2(rulerEnd.x, rulerEnd.y), 4.f, light_red);

            const Vec2f line(rulerEnd - rulerStart);
            const float lineLength = line.abs();
            Vec2f labelPos = 0.5f * Vec2f(rulerStart + rulerEnd);
            float angleDeg = 0.0f;
            if (lineLength > 0.001f) {
                Vec2f normal(-line.y / lineLength, line.x / lineLength);
                if (normal.y > 0.0f) {
                    normal = -normal;
                }

                constexpr float kLabelOffset = 16.0f;
                labelPos += normal * kLabelOffset;

                angleDeg = std::atan2(line.y, line.x) * 180.0f / std::numbers::pi;
                if (angleDeg > 90.0f) {
                    angleDeg -= 180.0f;
                }
                else if (angleDeg < -90.0f) {
                    angleDeg += 180.0f;
                }
            }
            else {
                labelPos.y -= 10.0f;
            }

            if (!rulerLabel.empty()) {
                const float angle_rad = angleDeg * std::numbers::pi_v<float> / 180.f;
                AddTextRotated(dl, ImGui::GetFont(), 18.f, labelPos, angle_rad, IM_COL32(235, 235, 235, 255), rulerLabel.data());
            }
        }
    }

    static void AddTextRotated(ImDrawList* dl, ImFont* font, float font_size, Vec2f pos, float angle_rad, ImU32 color,
                               std::string_view text) {
        if (text.empty()) {
            return;
        }

        const float scale = font_size / font->FontSize;
        const float cos_a = std::cos(angle_rad);
        const float sin_a = std::sin(angle_rad);

        const char* text_begin = text.data();
        const char* text_end = text.data() + text.size();
        const ImVec2 text_size = ImGui::GetFont()->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text_begin, text_end);

        pos.x -= (text_size.x * cos_a - text_size.y * sin_a) * 0.5f;
        pos.y -= (text_size.x * sin_a + text_size.y * cos_a) * 0.5f;

        ImVec2 cursor(pos.x, pos.y);

        for (const char* p = text_begin; p < text_end;) {
            unsigned int c;
            int char_len = ImTextCharFromUtf8(&c, p, text_end);
            if (char_len <= 0) {
                break;
            }
            p += char_len;

            const ImFontGlyph* glyph = font->FindGlyph((ImWchar)c);
            if (!glyph) {
                continue;
            }

            const float x0 = cursor.x + glyph->X0 * scale;
            const float y0 = cursor.y + glyph->Y0 * scale;
            const float x1 = cursor.x + glyph->X1 * scale;
            const float y1 = cursor.y + glyph->Y1 * scale;

            ImVec2 corners[4] = {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};

            for (auto& corner : corners) {
                const float dx = corner.x - pos.x;
                const float dy = corner.y - pos.y;
                corner.x = pos.x + dx * cos_a - dy * sin_a;
                corner.y = pos.y + dx * sin_a + dy * cos_a;
            }

            dl->PrimReserve(6, 4);
            dl->PrimQuadUV(corners[0], corners[1], corners[2], corners[3], {glyph->U0, glyph->V0}, {glyph->U1, glyph->V0},
                           {glyph->U1, glyph->V1}, {glyph->U0, glyph->V1}, color);

            cursor.x += glyph->AdvanceX * scale;
        }
    }
};
