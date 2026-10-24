#pragma once

#include <vector>

#include "RenderPass.hpp"
#include <Wgpu/Resources.hpp>

#include "Camera.hpp"
#include "RenderLayout.hpp"
#include "SceneObject.hpp"
#include "ViewportArea.hpp"

class TransformController;

/// A view of a scene: an area of the surface, a camera and the objects it draws.
/// Owns the camera uniform (bind group 0 of every object pipeline), so one object
/// can be drawn by several viewports with different cameras.
class Viewport final : public ViewportArea {
public:
    Viewport(RenderLayout& layout, Camera& camera);

    /// Write the camera and record every object into its rectangle.
    void render(RenderPass& pass, glm::uvec2 surfaceSize);

    Camera& camera() const noexcept { return camera_; }

    /// Objects drawn in order; composition assigns the current set.
    void setObjects(std::vector<SceneObject*> objects) { objects_ = std::move(objects); }
    const std::vector<SceneObject*>& objects() const noexcept { return objects_; }

    /// Controller of this viewport's camera (its child in the tree), assigned by composition.
    TransformController* controller = nullptr;

private:
    Wgpu::Device& device_;
    Camera& camera_;
    Wgpu::Buffer uniform_;
    Wgpu::BindingSet group_;
    std::vector<SceneObject*> objects_;
};
