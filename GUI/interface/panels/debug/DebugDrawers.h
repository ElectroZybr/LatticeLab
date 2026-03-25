#pragma once
#include <any>
#include <cmath>
#include <format>
#include "imgui.h"

#include "Engine/math/Vec2D.h"
#include "Engine/math/Vec3D.h"

namespace DebugDrawers {
    inline float toFloat(const std::any& a) {
        if (a.type() == typeid(float))  return std::any_cast<float>(a);
        if (a.type() == typeid(double)) return static_cast<float>(std::any_cast<double>(a));
        return 0.f;
    }

    inline void Float(std::string_view label, const std::any& a) {
        const float val = toFloat(a);
        ImGui::TextDisabled("%s", label.data());
        ImGui::SameLine();
        ImGui::Text("%s", std::format("{:.2f}", val).data());
    }

    inline void Int(std::string_view label, const std::any& a) {
        ImGui::TextDisabled("%s", label.data());
        ImGui::SameLine();

        if      (a.type() == typeid(uint8_t)) ImGui::Text("%d",  std::any_cast<uint8_t>(a));
        else if (a.type() == typeid(short))   ImGui::Text("%d",  std::any_cast<short>(a));
        else if (a.type() == typeid(int))     ImGui::Text("%d",  std::any_cast<int>(a));
        else if (a.type() == typeid(long))    ImGui::Text("%ld", std::any_cast<long>(a));
        else if (a.type() == typeid(size_t))  ImGui::Text("%zu", std::any_cast<size_t>(a));
    }

    inline void String(std::string_view label, const std::any& a) {
        ImGui::TextDisabled("%s", label.data());
        ImGui::SameLine();
        if (a.type() == typeid(std::string))
            ImGui::Text("%s", std::any_cast<const std::string&>(a).data());
        else if (a.type() == typeid(std::string_view))
            ImGui::Text("%s", std::any_cast<std::string_view>(a).data());
    }

    inline void Vec2D(std::string_view label, const std::any& a) {
        const auto& v = std::any_cast<const ::Vec2D&>(a);
        ImGui::TextDisabled("%s", label.data());
        ImGui::SameLine();
        ImGui::Text("%s", std::format("{:^+8.2f} {:^+8.2f}", v.x, v.y).data());
    }

    inline void Vec3D(std::string_view label, const std::any& a) {
        const auto& v = std::any_cast<const ::Vec3D&>(a);
        ImGui::TextDisabled("%s", label.data());
        ImGui::SameLine();
        ImGui::Text("%s", std::format("{:^+8.2f} {:^+8.2f} {:^+8.2f}", v.x, v.y, v.z).data());
    }
}