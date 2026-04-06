#pragma once

#include "Rendering/RendererGL.h"

class Renderer3D : public RendererGL {
public:
    Renderer3D(sf::RenderTarget& t, sf::View& gv, SimBox& simbox);
    ~Renderer3D() override = default;

protected:
    void updateMatrices() override;
    glm::vec3 getLightDir() override;
};
