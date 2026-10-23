#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <WGPU/CommandList.hpp>

struct Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

/// Camera uniform of a viewport (bind group 0, binding 0); WGSL mirrors this layout.
struct CameraUniform {
    glm::mat4 viewProjection;
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec4 position;
};

/// One viewport drawing: its rectangle, camera matrices and the camera bind group.
struct ViewState {
    WGPU::Rect rect;
    const CameraUniform& camera;
    WGPU::BindingSet& cameraGroup;
};

/// Object of a scene: a transform, and drawing into a viewport. Pipelines are built
/// on the window's RenderLayout and bind `cameraGroup` as group 0.
class SceneObject {
public:
    virtual ~SceneObject() = default;

    /// Record draw commands for one viewport. Objects without visuals draw nothing.
    virtual void render(WGPU::RenderPass&, const ViewState&) {}

    /// Model matrix of the transform.
    glm::mat4 model() const noexcept {
        return glm::translate(glm::mat4(1.0f), transform_.position) *
            glm::mat4_cast(transform_.rotation) * glm::scale(glm::mat4(1.0f), transform_.scale);
    }

    Transform& transform() noexcept { return transform_; }
    const Transform& transform() const noexcept { return transform_; }

    void move(const glm::vec3& delta) noexcept {
        transform_.position += delta;
    }

    void rotate(const glm::quat& delta) noexcept {
        transform_.rotation = glm::normalize(delta * transform_.rotation);
    }

    void scale(const glm::vec3& factor) noexcept {
        transform_.scale *= factor;
    }

    void setPosition(const glm::vec3& position) noexcept {
        transform_.position = position;
    }

    void setRotation(const glm::quat& rotation) noexcept {
        transform_.rotation = glm::normalize(rotation);
    }

    void setScale(const glm::vec3& scale) noexcept {
        transform_.scale = scale;
    }

private:
    Transform transform_;
};
