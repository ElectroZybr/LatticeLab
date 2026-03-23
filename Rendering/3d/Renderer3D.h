#pragma once

#include "../RendererGL.h"

class Renderer3D : public RendererGL {
public:
    Renderer3D(sf::RenderTarget& t, sf::View& gv);
    ~Renderer3D() override = default;

    void drawOverlay() override {};

    void setBoxContour(sf::Vector2i screenStart, sf::Vector2i screenEnd) override {}
    void setLassoContour(const std::vector<sf::Vector2i>& points) override {}

protected:
    void updateMatrices() override;
    glm::vec3 getLightDir() override;
};