#include "Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr sf::Color kSelectionContourColor(75, 75, 75); 
}

Renderer2D::Renderer2D(sf::RenderTarget& t, sf::View& gv)
    : RendererGL(t, gv)
{
    camera.setZoom(10.f);
    shaderProgram = linkProgram("assets/shaders/2d/atom.vert",
                                "assets/shaders/2d/atom.frag");
    boxShader = linkProgram("assets/shaders/3d/box.vert",
                            "assets/shaders/3d/box.frag");
    bondShader = linkProgram("assets/shaders/3d/bond.vert",
                             "assets/shaders/3d/bond.frag",
                             "assets/shaders/3d/bond.geom");
    gridShader = linkProgram("assets/shaders/3d/grid.vert",
                            "assets/shaders/3d/grid.frag");
}

void Renderer2D::updateMatrices() {
    const auto size = target.getSize();
    if (size.x == 0 || size.y == 0) return;

    float windowAspect = static_cast<float>(size.x) / static_cast<float>(size.y);

    float viewWidth = static_cast<float>(size.x) / camera.getZoom();
    float viewHeight = viewWidth / windowAspect;

    projection = glm::ortho(
        -viewWidth / 2.0f,  viewWidth / 2.0f,
         -viewHeight / 2.0f, viewHeight / 2.0f,
        -10000.f, 10000.f
    );

    view = glm::translate(glm::mat4(1.0f), glm::vec3(-camera.getPosition().x, -camera.getPosition().y, 0.0f));
}
