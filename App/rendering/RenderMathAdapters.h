#pragma once

#include <glm/glm.hpp>

#include "Engine/math/Ray.h"
#include "Engine/math/Vec2.h"
#include "Engine/math/Vec3.h"
#include "Rendering/RenderMath.h"

namespace App::Rendering {
    inline glm::vec2 toGlmVec2(const Vec2f& value) {
        return glm::vec2(value.x, value.y);
    }

    inline glm::ivec2 toGlmVec2(const Vec2i& value) {
        return glm::ivec2(value.x, value.y);
    }

    inline glm::vec3 toGlmVec3(const Vec3f& value) {
        return glm::vec3(value.x, value.y, value.z);
    }

    inline Vec2i toEngineVec2i(const glm::ivec2& value) {
        return Vec2i(value.x, value.y);
    }

    inline Vec2f toEngineVec2f(const glm::vec2& value) {
        return Vec2f(value.x, value.y);
    }

    inline Vec3f toEngineVec3f(const glm::vec3& value) {
        return Vec3f(value.x, value.y, value.z);
    }

    inline Ray toEngineRay(const RenderRay& value) {
        return Ray(toEngineVec3f(value.origin), toEngineVec3f(value.dir));
    }
}