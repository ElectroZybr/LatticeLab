#pragma once

#include <memory>
#include <vector>

#include "RenderPass.hpp"
#include <Wgpu/Resources.hpp>

#include "Camera.hpp"
#include "Light.hpp"
#include "RenderLayout.hpp"
#include "SceneObject.hpp"
#include "ViewportArea.hpp"

class TransformController;

/// Lights of a scene on the GPU (group 0, binding 1): the header, then `count` records.
/// WGSL mirrors both (`Lights` in the lit shaders).
struct LightsHeader {
    glm::vec4 ambient;      // rgb
    glm::uvec4 count;       // x
};
struct LightRecord {
    glm::vec4 position;     // point: xyz position, w 1; directional: xyz direction of the rays, w 0
    glm::vec4 color;        // rgb × intensity, w range (point)
};

/// A view of a scene: an area of the surface, a camera, the objects it draws and the
/// lights they are lit by. Owns group 0 of every object pipeline (camera and lights), so
/// one object can be drawn by several viewports with different cameras and lights.
/// Opaque objects are drawn first, then the transparent ones, each in order.
class Viewport final : public ViewportArea {
public:
    Viewport(RenderLayout& layout, Camera& camera);

    /// Write the camera and record every object into its rectangle.
    void render(RenderPass& pass, glm::uvec2 surfaceSize);

    Camera& camera() const noexcept { return camera_; }

    /// Objects drawn in order; composition assigns the current set.
    void setObjects(std::vector<SceneObject*> objects) { objects_ = std::move(objects); }
    const std::vector<SceneObject*>& objects() const noexcept { return objects_; }

    /// Lights of the scene (its children in the tree); composition assigns the current set.
    void setLights(std::vector<Light*> lights) { lights_ = std::move(lights); }
    const std::vector<Light*>& lights() const noexcept { return lights_; }

    /// Light reaching every surface from all sides.
    glm::vec3 ambient{0.12f};

    /// Controller of this viewport's camera (its child in the tree), assigned by composition.
    TransformController* controller = nullptr;

private:
    void uploadLights();

    RenderLayout& layout_;
    Wgpu::Device& device_;
    Camera& camera_;
    Wgpu::Buffer uniform_;
    std::unique_ptr<Wgpu::Buffer> lightBuffer_;
    size_t lightCapacity_ = 0;
    std::unique_ptr<Wgpu::BindingSet> group_;
    std::vector<SceneObject*> objects_;
    std::vector<Light*> lights_;
    std::vector<std::byte> lightBytes_;
};
