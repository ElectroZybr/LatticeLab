#include "Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Engine/Tools.h"

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

void Renderer2D::setBoxContour(sf::Vector2i scrennStart, sf::Vector2i screenEnd) {
    sf::Vector2f start = Tools::screenToWorld(scrennStart);
    sf::Vector2f end = Tools::screenToWorld(screenEnd);

    // Вычисляем размер и позицию (учитываем, что start может быть больше end)
    float left = std::min(start.x, end.x);
    float top = std::min(start.y, end.y);
    float width = std::abs(end.x - start.x);
    float height = std::abs(end.y - start.y);

    boxShape.setPosition({left, top});
    boxShape.setSize({width, height});

    boxShape.setFillColor(sf::Color::Transparent);
    boxShape.setOutlineColor(kSelectionContourColor);
    boxShape.setOutlineThickness(1.0f / camera.getZoom());
}

void Renderer2D::setLassoContour(const std::vector<sf::Vector2i>& points) {
    lassoShape.setPrimitiveType(sf::PrimitiveType::LineStrip);
    lassoShape.clear();

    for (const auto& p : points) {
        lassoShape.append(sf::Vertex(Tools::screenToWorld(p), kSelectionContourColor));
    }

    if (!points.empty()) {
        lassoShape.append(sf::Vertex(Tools::screenToWorld(points[0]), kSelectionContourColor));
    }
}

void Renderer2D::drawOverlay() {
    if (!isBoxVisible && !isLassoVisible) {
        return;
    }

    // Важно: переключаемся на SFML отрисовку поверх OpenGL
    target.pushGLStates();

    if (isBoxVisible) {
        target.draw(boxShape);
    }

    if (isLassoVisible) {
        target.draw(lassoShape);
    }

    target.popGLStates();
}
