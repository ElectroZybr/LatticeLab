#pragma once

#include "../RendererGL.h"

class Renderer2D : public RendererGL {
public:
    Renderer2D(sf::RenderTarget& t, sf::View& gv);
    ~Renderer2D() override = default;

    void setBoxContour(sf::Vector2i screenStart, sf::Vector2i screenEnd) override;
    void setLassoContour(const std::vector<sf::Vector2i>& screenPoints) override;

    void drawOverlay() override;
protected:
    bool useLighting() override { return false; }
    void updateMatrices() override;
    glm::vec3 getLightDir() override { return glm::vec3(0.f); };

    sf::RectangleShape boxShape;
    sf::VertexArray lassoShape;
};