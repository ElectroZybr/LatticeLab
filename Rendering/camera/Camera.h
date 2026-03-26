#pragma once

#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>

#include "Engine/math/Ray.h"

struct ScreenPoint {
    Vec2f pos;
    bool  visible;
};

class Camera {
    friend class Mouse;
    friend class Keyboard;
private:
    sf::View* view;
    sf::Vector2f position;
    float zoom;
    float speed;
    float moveSpeed;
    float zoomSpeed;

    bool isDragging;
    sf::Vector2f lastMousePos;

    sf::Vector2i dragStartPixelPos;
    sf::Vector2f dragStartCameraPos;

    Mode mode = Mode::Mode2D;

    // Orbit / Free
    float azimuth   = 0.f;
    float elevation = 0.f;

    // Перспектива
    static constexpr float FOV      = 45.f;
    static constexpr float NEAR     = 0.1f;
    static constexpr float FAR      = 10000.f;

public:
    Camera(sf::View* view, float moveSpeed = 500.f, float zoomSpeed = 0.1f);

    void update(sf::RenderTarget& target);

    void move(float offsetX, float offsetY);

    void setPosition(float x, float y);

    void setMode(CameraMode newMode) { mode = newMode; }
    CameraMode getMode() const { return mode; }

    // Обратная совместимость
    void setOrbitMode(bool enabled) {
        mode = enabled ? CameraMode::Orbit : CameraMode::Mode2D;
    }

    sf::Vector2f getPosition() const;

    void orbitDrag(sf::Vector2i delta);
    void freeDrag(sf::Vector2i delta);  // для Free mode

    void zoomAt(float factor, sf::Vector2f mousePos, sf::RenderWindow& window);

    float getZoom() const { return zoom; }
    void  setZoom(float new_zoom);

    sf::View&       getView()       { return *view; }
    const sf::View& getView() const { return *view; }

    glm::vec3 getEyePosition() const;
    glm::mat4 getViewMatrix()  const;
    glm::mat4 getProjectionMatrix(float screenWidth, float screenHeight) const;

    Ray screenToRay(float screenX, float screenY, float screenWidth, float screenHeight) const;
    ScreenPoint worldToScreen(const Vec3f& worldPos, float screenWidth, float screenHeight) const;

    enum class Mode {
        Mode2D,
        Orbit,
        Free
    };
};