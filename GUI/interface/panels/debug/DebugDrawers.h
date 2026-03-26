#pragma once
#include <any>
#include <cmath>
#include "imgui.h"

#include "Engine/math/Vec2f.h"
#include "Engine/math/Vec3f.h"

namespace DebugDrawers {
    inline float toFloat(const std::any& a) {
        if (a.type() == typeid(float))  return std::any_cast<float>(a);
        if (a.type() == typeid(double)) return static_cast<float>(std::any_cast<double>(a));
        return 0.f;
    }

    template <int precision>
    inline void Float(std::string_view label, const std::any& a) {
        const float val = toFloat(a);
        ImGui::TextDisabled("%s", label.data());
        ImGui::SameLine();
        ImGui::Text("%.*f", precision, val);
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

    template <int precision>
    inline void Vec2f(std::string_view label, const std::any& a) {
        const auto& v = std::any_cast<const ::Vec2f&>(a);
        ImGui::TextDisabled("%s", label.data());
        ImGui::SameLine();
        ImGui::Text("%+.*f %+.*f", precision, v.x, precision, v.y);
    }

    template <int precision>
    inline void Vec3f(std::string_view label, const std::any& a) {
        const auto& v = std::any_cast<const ::Vec3f&>(a);
        ImGui::TextDisabled("%s", label.data());
        ImGui::SameLine();
        ImGui::Text("%+.*f %+.*f %+.*f", precision, v.x, precision, v.y, precision, v.z);
    }
}
