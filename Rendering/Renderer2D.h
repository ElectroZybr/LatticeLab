#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "Rendering/Renderer.h"
#include "generated/shaders/atom2d.wgsl.h"

class Renderer2D : public RendererWGPU {
public:
    Renderer2D() {
        initAtomSpherePipeline(atom2dWGSL);
        initSharedPipelines();

        camera.setMode(Camera::Mode::Mode2D);
        camera.resetView();
    }

    ~Renderer2D() override = default;

protected:
    bool useLighting() override { return false; }

    void updateMatrices() override {
        const float aspect = static_cast<float>(camera.screenSize.x) / static_cast<float>(camera.screenSize.y);
        const float viewWidth = static_cast<float>(camera.screenSize.x) / camera.getZoom();
        const float viewHeight = viewWidth / aspect;

        projection = glm::orthoRH_ZO(-viewWidth / 2.f, viewWidth / 2.f, -viewHeight / 2.f, viewHeight / 2.f, -10000.f, 10000.f);
        view = glm::translate(glm::mat4(1.f), glm::vec3(-camera.getPosition().x, -camera.getPosition().y, 0.f));
    }

    glm::vec3 getLightDir() override { return glm::vec3(0.f); }
};
