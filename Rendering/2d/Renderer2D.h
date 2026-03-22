#pragma once

#include <SFML/Graphics.hpp>

#include "../BaseRenderer.h"
#include "Engine/physics/SpatialGrid.h"
#include "Engine/physics/Bond.h"

class Renderer2D : public IRenderer {
public:
    Renderer2D(sf::RenderTarget& window, sf::View& gameView, sf::View& uiView);

    void drawShot(const std::vector<Atom>& atoms,
                  const SimBox& box, float deltaTime) override;
    void setSelectionFrame(Vec2D start, Vec2D end, float scale) override;
    void setLassoContour(const std::vector<Vec2D>& points, float scale) override;
    void wallImage(Vec3D start, Vec3D end) override;
    void showSelectionFrame(bool show) override { drawSelectionFrame = show; }
    void showLassoContour(bool show) override { drawLassoContour = show; }

    sf::Texture forceTexture;

private:
    sf::RenderTarget& target;
    sf::View& gameView;
    sf::View& uiView;

    std::vector<sf::Vertex> gridLines;
    sf::Texture atomTextureLow;
    sf::Texture atomTextureMid;
    sf::Texture atomTextureHigh;
    sf::VertexArray atomBatch{sf::PrimitiveType::Triangles};
    std::vector<sf::Vertex> bondBatch;
    std::vector<const Atom*> sortedAtoms;
    sf::RectangleShape frameShape;
    sf::VertexArray lassoContour{sf::PrimitiveType::LineStrip};
    sf::RectangleShape forceFieldQuad;
    sf::Shader forceFieldShader;
    bool forceFieldShaderLoaded = false;
    bool drawSelectionFrame     = false;
    bool drawLassoContour       = false;

    void initAtomTexture(sf::Texture& texture, unsigned texSize);
    void drawTransparencyMap(sf::RenderTarget& target, const SpatialGrid& grid);
    void drawForceField(const sf::Texture& forceTexture, const SimBox& box);
    int getWallForce(int coord, int min, int max);
};