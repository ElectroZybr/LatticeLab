#pragma once

#include <vector>

#include "Rendering/camera/Camera.h"
#include "Engine/physics/Atom.h"
#include "Engine/SimBox.h"
#include "Engine/math/Vec2D.h"
#include "Engine/math/Vec3D.h"

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void drawShot(const std::vector<Atom>& atoms,
                          const SimBox& box, float deltaTime) = 0;
    virtual void setSelectionFrame(Vec2D start, Vec2D end, float scale) = 0;
    virtual void setLassoContour(const std::vector<Vec2D>& points, float scale) = 0;
    virtual void wallImage(Vec3D start, Vec3D end) = 0;
    virtual void showSelectionFrame(bool show) = 0;
    virtual void showLassoContour(bool show) = 0;

    bool drawGrid           = false;
    bool drawBonds          = false;
    bool speedGradient      = false;
    bool speedGradientTurbo = false;
    float drawBondsZoom     = 25.f;
    float alpha             = 0.05f;

    Camera camera;

protected:
IRenderer(sf::View& gv)
    : camera(&gv) {}
};
