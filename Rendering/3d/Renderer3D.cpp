#include "Renderer3D.h"
#include <glm/gtc/matrix_transform.hpp>

Renderer3D::Renderer3D(sf::RenderTarget& t, sf::View& gv)
    : RendererGL(t, gv)
{
    camera.setMode(Camera::Mode::Orbit);
    camera.setZoom(4.f);

    atomShaders[0] = linkProgram("assets/shaders/3d/atom.vert",
                                 "assets/shaders/3d/atom.frag",
                                 "",
                                 "#define COLOR_MODE 0");
    atomShaders[1] = linkProgram("assets/shaders/3d/atom.vert",
                                 "assets/shaders/3d/atom.frag",
                                 "",
                                 "#define COLOR_MODE 1");
    atomShaders[2] = linkProgram("assets/shaders/3d/atom.vert",
                                 "assets/shaders/3d/atom.frag",
                                 "",
                                 "#define COLOR_MODE 2");
    atomShader = atomShaders[0];
    boxShader = linkProgram("assets/shaders/3d/box.vert",
                            "assets/shaders/3d/box.frag");
    bondShader = linkProgram("assets/shaders/3d/bond.vert",
                             "assets/shaders/3d/bond.frag",
                             "assets/shaders/3d/bond.geom");
    gridShader = linkProgram("assets/shaders/3d/grid.vert",
                            "assets/shaders/3d/grid.frag");

    initAtomColors();
}

void Renderer3D::updateMatrices() {
    const auto size = target.getSize();
    const float aspect = static_cast<float>(size.x) / static_cast<float>(size.y);

    projection = glm::perspective(glm::radians(45.f), aspect, 0.1f, 1000.f);
    view = camera.getViewMatrix();
}

glm::vec3 Renderer3D::getLightDir() {
    const glm::vec3 eye = camera.getEyePosition();
    return glm::normalize(
        glm::vec3(view * glm::vec4(eye, 0.f)) + glm::vec3(25.f, 25.f, 0.f)
    );
}
