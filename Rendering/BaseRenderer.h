#pragma once

#include <cstdint>

#include "Rendering/camera/Camera.h"
#include "Engine/physics/AtomStorage.h"
#include "Engine/physics/Bond.h"
#include "Engine/SimBox.h"

class IRenderer {
public:
    enum class SpeedColorMode : uint8_t {
        AtomColor = 0,
        GradientClassic = 1,
        GradientTurbo = 2,
    };

    virtual ~IRenderer() = default;

    virtual void drawShot(const AtomStorage& atoms,
                          const SimBox& box) = 0;

    void setAtomStorage(const AtomStorage* storage) { atomStorage = storage; }
    void setBondStorage(const Bond::List* storage) { bondStorage = storage; }

    bool drawGrid           = false;
    bool drawBonds          = false;
    SpeedColorMode speedColorMode = SpeedColorMode::AtomColor;
    float speedGradientMax  = 5.0f;
    float alpha             = 0.05f;

    Camera camera;

protected:
    IRenderer(sf::View& gv, SimBox& box)
        : camera(&gv, box) {}
    const AtomStorage* atomStorage = nullptr;
    const Bond::List* bondStorage = nullptr;
};
