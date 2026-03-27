#pragma once

#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>

#include "Engine/math/Ray.h"

class Camera {
    friend class Mouse;
    friend class Keyboard;
public:
    enum class Mode: uint8_t {
        Mode2D,
        Orbit,
        Free
    };

    Camera(sf::View* view, float moveSpeed = 500.f, float zoomSpeed = 0.1f);

    void update(sf::RenderTarget& target);

    void move(Vec2f offset) { position += offset; }
    void move3D(Vec3f offset) { freePosition += offset; }

    void setPosition(Vec2f pos) { position = pos; };
    Vec2f getPosition() const { return position; };

    void setMode(Mode newMode) { mode = newMode; }
    Mode getMode() const { return mode; }

    void orbitDrag(sf::Vector2i delta);
    void freeDrag(sf::Vector2i delta);  // для Free mode

    Vec3f screenToWorld(sf::Vector2i screenPos) const;
    sf::Vector2i worldToScreen(Vec3f worldPos) const;

    void zoomAt(float factor, sf::Vector2f mousePos, sf::RenderWindow& target);
    float getZoom() const { return zoom; }
    void  setZoom(float new_zoom);

    sf::View&       getView()       { return *view; }
    const sf::View& getView() const { return *view; }

    glm::vec3 getEyePosition() const;
    glm::mat4 getViewMatrix()  const;
    glm::mat4 getProjectionMatrix() const;

    Ray screenToRay(float screenX, float screenY) const;

private:
    sf::Vector2f screenSize;
    sf::View* view;
    Vec2f position;
    Vec3f freePosition{0.f, 0.f, -50.f};
    float zoom;
    float speed;
    float moveSpeed;
    float zoomSpeed;

    bool isDragging;
    Vec2f lastMousePos;

    sf::Vector2i dragStartPixelPos;
    Vec2f dragStartCameraPos;

    Mode mode = Mode::Mode2D;

    // Orbit / Free
    float azimuth   = 0.f;
    float elevation = 0.f;

    // Перспектива
    static constexpr float FOV      = 45.f;
    static constexpr float NEAR     = 0.1f;
    static constexpr float FAR      = 10000.f;
};