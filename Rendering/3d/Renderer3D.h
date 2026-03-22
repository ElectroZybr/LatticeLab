#pragma once

#include <string_view>
#include <vector>

#include <glad/glad.h>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>

#include "../BaseRenderer.h"

class Renderer3D : public IRenderer {
public:
    Renderer3D(sf::RenderTarget& target, sf::View& gameView, sf::View& uiView);
    ~Renderer3D() override;

    void initBoxGL();
    void drawBox(const SimBox& box);

    void drawShot(const std::vector<Atom>& atoms,
                  const SimBox& box, float deltaTime) override;
    void setSelectionFrame(Vec2D start, Vec2D end, float scale) override;
    void setLassoContour(const std::vector<Vec2D>& points, float scale) override;
    void wallImage(Vec3D start, Vec3D end) override;

    void showSelectionFrame(bool show) override;
    void showLassoContour(bool show) override;
private:
    sf::RenderTarget& target;
    sf::View& uiView;

    // OpenGL объекты
    GLuint vao         = 0;
    GLuint quadVbo     = 0; // billboard квад
    GLuint instanceVbo = 0; // данные атомов (pos, radius, color)
    GLuint shaderProgram = 0;

    GLuint boxVao = 0;
    GLuint boxVbo = 0;
    GLuint boxShader = 0;

    // инстансинг буфер
    struct AtomInstance {
        glm::vec3 pos;
        float     radius;
        glm::vec3 color;
        float     pad = 0.f; // выравнивание
    };
    std::vector<AtomInstance> instanceData;

    glm::mat4 projection;
    glm::mat4 view;

    void initGL();
    void initShaders();
    void updateMatrices();
    GLuint compileShader(GLenum type, std::string_view src);
    GLuint loadShader(GLenum type, std::string_view path);
};
