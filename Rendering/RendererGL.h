#pragma once

#include "BaseRenderer.h"

#include <SFML/Graphics.hpp>
#include <glad/glad.h>


class RendererGL : public IRenderer {
public:
    RendererGL(sf::RenderTarget& t, sf::View& gv);
    virtual ~RendererGL();

    void drawShot(const std::vector<Atom>& atoms,
                  const SimBox& box) override;

protected:
    virtual bool useLighting() { return true; }
    virtual void updateMatrices() = 0;
    virtual glm::vec3 getLightDir() = 0;

    void initGL();
    void initBondGL();
    void initGridGL();
    void initBoxGL();

    GLuint loadShader(GLenum type, std::string_view path);
    GLuint compileShader(GLenum type, std::string_view src);
    GLuint linkProgram(std::string_view vert, std::string_view frag,
                       std::string_view geom = "");

    void drawBox(const SimBox& box);
    void drawBondsGL(const glm::vec3& boxOffset);
    void drawGridGL(const SpatialGrid& grid, const glm::vec3& boxOffset);

    // общее состояние
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};

    // GL handles
    GLuint vao{0},        quadVbo{0},     instanceVbo{0}, shaderProgram{0};
    GLuint boxVao{0},     boxVbo{0},      boxShader{0};
    GLuint bondVao{0},    bondVbo{0},     bondShader{0};
    GLuint gridVao{0},    gridLineVbo{0}, gridInstVbo{0},  gridShader{0};

    struct alignas(32) AtomInstance {
        glm::vec3 pos;
        glm::vec3 color;
        float radius;
        float isSelected;
    };

    struct alignas(32) BondInstance {
        glm::vec3 posA;
        glm::vec3 posB;
        float radius;
    };

    struct alignas(16) GridInstance {
        glm::vec3 origin;
        float cellSize;
        float atomCount;
    };

    std::vector<AtomInstance> instanceData;
    std::vector<BondInstance> bondData;
    std::vector<GridInstance> gridData;

    sf::RenderTarget& target;
    const SimBox* currentBox = nullptr;
    const std::vector<Atom>* currentAtoms = nullptr;
};
