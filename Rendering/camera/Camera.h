#pragma once

#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>

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

    bool orbitMode = false;
    float azimuth   = 0.f;
    float elevation = 1.0f; 
public:
    Camera(sf::View* view, float moveSpeed = 500.f, float zoomSpeed = 0.1f);
    
    void update(float deltaTime, sf::RenderTarget& target);
    
    void move(float offsetX, float offsetY);
    
    void setPosition(float x, float y);
    void setOrbitMode(bool enabled) { orbitMode = enabled; }
    
    sf::Vector2f getPosition() const;

    void orbitDrag(sf::Vector2i delta);

    void zoomAt(float factor, sf::Vector2f mousePos, sf::RenderWindow& window);
    
    float getZoom() const;

    void setZoom(float new_zoom);
    
    const sf::View& getView() const;

    glm::vec3 getEyePosition() const;
    glm::mat4 getViewMatrix() const;
    // void resize(sf::Vector2f newSize);
};