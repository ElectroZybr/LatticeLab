#pragma once

#include "BaseRenderer.h"

#include <array>
#include <SFML/Graphics.hpp>
#include <glad/glad.h>


class RendererGL : public IRenderer {
public:
    RendererGL(sf::RenderTarget& t, sf::View& gv, SimBox& simbox);
    virtual ~RendererGL();

    void drawShot(const AtomStorage& atoms,
                  const SimBox& box) override;

protected:
    virtual bool useLighting() { return true; }
    virtual void updateMatrices() = 0;
    virtual glm::vec3 getLightDir() = 0;

    void initQuadGL();
    void initAtomGL();
    void initBondGL();
    void initGridGL();
    void initBoxGL();

    void initAtomColors();

    GLuint loadShader(GLenum type, std::string_view path, std::string_view defines = {});
    GLuint compileShader(GLenum type, std::string_view src);
    GLuint linkProgram(std::string_view vert, std::string_view frag,
                       std::string_view geom = "",
                       std::string_view vertDefines = {});

    void drawAtoms(const AtomStorage& atoms, const SimBox& box);
    void drawBox(const SimBox& box);
    void drawBondsGL();
    void drawGridGL(const SpatialGrid& grid);
    GLuint atomShaderForMode(SpeedColorMode mode) const;

    // общее состояние
    std::size_t lastAtomCount = 0;
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};

    // GL handles
    GLuint quadVbo = 0;
    GLuint atomVao{0},    atomVbo{0},     atomShader{0};
    std::array<GLuint, 3> atomShaders{{0, 0, 0}};
    GLuint boxVao{0},     boxVbo{0},      boxShader{0};
    GLuint bondVao{0},    bondVbo{0},     bondShader{0};
    GLuint gridVao{0},    gridLineVbo{0}, gridInstVbo{0},  gridShader{0};

    // Атомы
    std::vector<float> radii;
    std::vector<uint8_t> selectedDataBuffer;

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

    std::vector<BondInstance> bondData;
    std::vector<GridInstance> gridData;

    sf::RenderTarget& target;
    const SimBox* currentBox = nullptr;
};
