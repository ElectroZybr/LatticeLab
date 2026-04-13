#pragma once

#include "Rendering/RendererBGFX.h"

class Renderer2DBGFX : public RendererBGFX {
public:
    Renderer2DBGFX(sf::RenderTarget& t, sf::View& gv, SimBox& simbox);
    ~Renderer2DBGFX() override = default;

protected:
    bool useLighting() override { return false; }
    void updateMatrices() override;
    glm::vec3 getLightDir() override { return glm::vec3(0.f); }
};
