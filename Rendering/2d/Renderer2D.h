#pragma once

#include "../RendererGL.h"

class Renderer2D : public RendererGL {
public:
    Renderer2D(sf::RenderTarget& t, sf::View& gv);
    ~Renderer2D() override = default;

protected:
    bool useLighting() override { return false; }
    void updateMatrices() override;
    glm::vec3 getLightDir() override { return glm::vec3(0.f); };
};