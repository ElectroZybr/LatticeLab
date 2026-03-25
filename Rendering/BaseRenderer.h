#pragma once

#include <vector>

#include "Rendering/camera/Camera.h"
#include "Engine/physics/AtomData.h"
#include "Engine/physics/AtomStorage.h"
#include "Engine/SimBox.h"

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void drawShot(const AtomStorage& atoms,
                          const SimBox& box) = 0;
    
    virtual void drawOverlay() = 0;
    virtual void setBoxContour(sf::Vector2i screenStart, sf::Vector2i screenEnd) = 0;
    virtual void setLassoContour(const std::vector<sf::Vector2i>& screenPoints) = 0;

    virtual void showBoxContour(bool show) { isBoxVisible = show; }
    virtual void showLassoContour(bool show) { isLassoVisible = show; }
    void setAtomStorage(const AtomStorage* storage) { atomStorage = storage; }

    bool drawGrid           = false;
    bool drawBonds          = false;
    bool speedGradient      = false;
    bool speedGradientTurbo = false;
    float speedGradientMax  = 5.0f; // 0.0f = auto
    float drawBondsZoom     = 25.f;
    float alpha             = 0.05f;

    bool isBoxVisible = false;
    bool isLassoVisible = false;

    Camera camera;

protected:
    IRenderer(sf::View& gv)
        : camera(&gv) {}
    const AtomStorage* atomStorage = nullptr;

    sf::Color turboColor(float t) {
        t = std::clamp(t, 0.f, 1.f);
        const float r = 34.61f + t * (1172.33f + t * (-10793.56f + t * (33300.12f + t * (-38394.49f + t * 14825.05f))));
        const float g = 23.31f + t * (557.33f + t * (1225.33f + t * (-3574.96f + t * (1073.77f + t * 707.56f))));
        const float b = 27.20f + t * (3211.10f + t * (-15327.97f + t * (27814.00f + t * (-22569.18f + t * 6838.66f))));;
        return sf::Color(
            std::clamp<uint8_t>(r, 0, 255),
            std::clamp<uint8_t>(g, 0, 255),
            std::clamp<uint8_t>(b, 0, 255))
        ;
    }
};
