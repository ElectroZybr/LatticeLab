#pragma once

#include "Rendering/RendererBGFX.h"

class Renderer3DBGFX : public RendererBGFX {
public:
    Renderer3DBGFX(SimBox& simbox);
    ~Renderer3DBGFX() override = default;

protected:
    void updateMatrices() override;
    glm::vec3 getLightDir() override;
    bool useLighting() override { return true; }
};
