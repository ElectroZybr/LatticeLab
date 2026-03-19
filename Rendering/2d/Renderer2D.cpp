#include "Renderer2D.h"

#include "imgui-SFML.h"

#include <algorithm>
#include <cmath>

namespace {
    sf::Color turboColor(float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        const float r = 34.61f + t * (1172.33f + t * (-10793.56f + t * (33300.12f + t * (-38394.49f + t * 14825.05f))));
        const float g = 23.31f + t * (557.33f + t * (1225.33f + t * (-3574.96f + t * (1073.77f + t * 707.56f))));
        const float b = 27.20f + t * (3211.10f + t * (-15327.97f + t * (27814.00f + t * (-22569.18f + t * 6838.66f))));

        return sf::Color(
            static_cast<std::uint8_t>(std::clamp(r, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(g, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(b, 0.0f, 255.0f))
        );
    }
}

Renderer2D::Renderer2D(sf::RenderWindow& w, sf::View& gv, sf::View& uv)
    : window(w), gameView(gv), uiView(uv), IRenderer(w, gv) {
    const int gridSize = 50;
    for (int x = -1000; x <= 1000; x += gridSize) {
        gridLines.emplace_back(sf::Vector2f(x, -1000), sf::Color(60, 60, 60));
        gridLines.emplace_back(sf::Vector2f(x, 1000), sf::Color(60, 60, 60));
    }
    for (int y = -1000; y <= 1000; y += gridSize) {
        gridLines.emplace_back(sf::Vector2f(-1000, y), sf::Color(60, 60, 60));
        gridLines.emplace_back(sf::Vector2f(1000, y), sf::Color(60, 60, 60));
    }

    forceFieldShaderLoaded = forceFieldShader.loadFromFile("Rendering/2d/shaders/force_shader.frag", sf::Shader::Type::Fragment);
    forceFieldQuad.setPosition(sf::Vector2f(0.f, 0.f));

    initAtomTexture(atomTextureLow, 16);
    initAtomTexture(atomTextureMid, 32);
    initAtomTexture(atomTextureHigh, 256);
    bondBatch.reserve(8192);
    sortedAtoms.reserve(16384);
}

void Renderer2D::initAtomTexture(sf::Texture& texture, unsigned texSize) {
    sf::Image image({texSize, texSize}, sf::Color(255, 255, 255, 0));

    const float center = (texSize - 1) * 0.5f;
    const float radius = center - 1.0f;
    const float radiusSq = radius * radius;

    for (unsigned y = 0; y < texSize; ++y) {
        for (unsigned x = 0; x < texSize; ++x) {
            const float dx = static_cast<float>(x) - center;
            const float dy = static_cast<float>(y) - center;
            if (dx * dx + dy * dy <= radiusSq) {
                image.setPixel({x, y}, sf::Color::White);
            }
        }
    }

    if (!texture.loadFromImage(image)) {
        return;
    }
    // if (smooth) {
    //     texture.generateMipmap();
    // }
}

void Renderer2D::wallImage(const Vec3D start, const Vec3D end) {
    constexpr int textureScale = 4;
    const double worldWidth = std::max(1.0, end.x - start.x);
    const double worldHeight = std::max(1.0, end.y - start.y);
    const unsigned int width = static_cast<unsigned int>(worldWidth * textureScale);
    const unsigned int height = static_cast<unsigned int>(worldHeight * textureScale);

    std::vector<std::uint8_t> forcePixels(width * height * 4, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int idx = 4 * (y * width + x);
            forcePixels[idx + 0] = 255;
            forcePixels[idx + 1] = 0;
            forcePixels[idx + 2] = 0;

            int alphaValue = getWallForce(x, 0, width - 1) + getWallForce(y, 0, height - 1);
            if (alphaValue > 255) alphaValue = 255;
            forcePixels[idx + 3] = static_cast<std::uint8_t>(alphaValue);
        }
    }

    if (!forceTexture.resize({width, height})) {
        return;
    }
    forceTexture.update(forcePixels.data());
    forceTexture.setSmooth(true);
    // forceTexture.generateMipmap();
}

int Renderer2D::getWallForce(int coord, int min, int max) {
    constexpr int border = 7;
    constexpr double k = 255.0 / static_cast<double>(border * border);
    double force = 0.0;

    if (coord < min + border) {
        const double dist = static_cast<double>((min + border) - coord);
        force += k * dist * dist;
    }

    if (coord > max - border) {
        const double dist = static_cast<double>(coord - (max - border));
        force += k * dist * dist;
    }

    if (force > 255.0) force = 255.0;
    return static_cast<int>(force);
}

void Renderer2D::drawShot(const std::vector<Atom>& atoms, const SimBox& box, float deltaTime) {
    // 1: 7000 мкс - отрисовка 225 атомов
    // 2: оптимизация батч на gpu. 1000 мкс - отрисовка 961 атома
    camera.update(deltaTime, window);

    window.clear(sf::Color(35, 35, 35, 255));
    window.setView(gameView);

    window.draw(&gridLines[0], gridLines.size(), sf::PrimitiveType::Lines);
    drawForceField(forceTexture, box);

    if (drawGrid) {
        drawTransparencyMap(window, box.grid);
    }

    sortedAtoms.clear();
    sortedAtoms.reserve(atoms.size());
    for (const Atom& a : atoms) sortedAtoms.emplace_back(&a);
    std::ranges::sort(sortedAtoms, [](const Atom* a, const Atom* b) { return a->coords.z > b->coords.z; });

    const sf::Vector2f boxOffset(static_cast<float>(box.start.x), static_cast<float>(box.start.y));
    const sf::Vector2f viewCenter = gameView.getCenter();
    const sf::Vector2f viewSize = gameView.getSize();
    const sf::FloatRect viewRect(
        viewCenter - viewSize * 0.5f,
        viewSize
    );

    const float zoom = camera.getZoom();
    const sf::Texture* activeAtomTexture = &atomTextureLow;
    if (zoom > 20.f) {
        activeAtomTexture = &atomTextureHigh;
    } else if (zoom > 4.f) {
        activeAtomTexture = &atomTextureMid;
    }

    atomBatch.clear();
    const sf::Vector2u texSize = activeAtomTexture->getSize();
    const sf::Vector2f uv00(0.f, 0.f);
    const sf::Vector2f uv10(static_cast<float>(texSize.x), 0.f);
    const sf::Vector2f uv11(static_cast<float>(texSize.x), static_cast<float>(texSize.y));
    const sf::Vector2f uv01(0.f, static_cast<float>(texSize.y));

    // // перед циклом по sortedAtoms
    // float maxSpeedForColor = 1.0f;
    // if (speedGradient) {
    //     maxSpeedForColor = 0.0f;
    //     for (const Atom* atom : sortedAtoms) {
    //         maxSpeedForColor = std::max(maxSpeedForColor, static_cast<float>(atom->speed.length()));
    //     }
    //     if (maxSpeedForColor < 1e-6f) {
    //         maxSpeedForColor = 1.0f;
    //     }
    // }

    for (const Atom* atom : sortedAtoms) {
        const float x = static_cast<float>(atom->coords.x) + boxOffset.x;
        const float y = static_cast<float>(atom->coords.y) + boxOffset.y;
        const float radius = std::max(0.1f, static_cast<float>(atom->getProps().radius - (atom->coords.z * alpha)));
        const float size = radius * 2.0f;
        const float outline = std::max(0.03f, radius * 0.03f);
        const bool drawOutline = (size * zoom >= 3.0f);
        const float outerX = x - outline;
        const float outerY = y - outline;
        const float outerSize = size + outline * 2.0f;

        const float cullX = drawOutline ? outerX : x;
        const float cullY = drawOutline ? outerY : y;
        const float cullSize = drawOutline ? outerSize : size;

        if (cullX + cullSize < viewRect.position.x || cullX > viewRect.position.x + viewRect.size.x ||
            cullY + cullSize < viewRect.position.y || cullY > viewRect.position.y + viewRect.size.y) {
            continue;
        }

        sf::Color color = atom->isSelect ? sf::Color::Red : sf::Color::Black;
        if (drawOutline) {
            atomBatch.append(sf::Vertex(sf::Vector2f(outerX, outerY),                         color, uv00));
            atomBatch.append(sf::Vertex(sf::Vector2f(outerX + outerSize, outerY),             color, uv10));
            atomBatch.append(sf::Vertex(sf::Vector2f(outerX + outerSize, outerY + outerSize), color, uv11));
            atomBatch.append(sf::Vertex(sf::Vector2f(outerX, outerY),                         color, uv00));
            atomBatch.append(sf::Vertex(sf::Vector2f(outerX + outerSize, outerY + outerSize), color, uv11));
            atomBatch.append(sf::Vertex(sf::Vector2f(outerX, outerY + outerSize),             color, uv01));
        }
        if (speedGradient){
            const float t = std::clamp(static_cast<float>(atom->speed.length()) / 5, 0.0f, 1.0f); // 0..1
            if (speedGradientTurbo) {
                color = turboColor(t);
            } else {
                color = sf::Color(static_cast<std::uint8_t>(255.0f * t), 0, static_cast<std::uint8_t>(255.0f * (1.0f - t))); // old: blue -> red
            }
        } else {
            color = atom->getProps().color;
        }
        atomBatch.append(sf::Vertex(sf::Vector2f(x, y),               color, uv00));
        atomBatch.append(sf::Vertex(sf::Vector2f(x + size, y),        color, uv10));
        atomBatch.append(sf::Vertex(sf::Vector2f(x + size, y + size), color, uv11));
        atomBatch.append(sf::Vertex(sf::Vector2f(x, y),               color, uv00));
        atomBatch.append(sf::Vertex(sf::Vector2f(x + size, y + size), color, uv11));
        atomBatch.append(sf::Vertex(sf::Vector2f(x, y + size),        color, uv01));
    }

    if (atomBatch.getVertexCount() > 0) {
        sf::RenderStates atomStates;
        atomStates.texture = activeAtomTexture;
        window.draw(atomBatch, atomStates);
    }

    if (drawBonds && camera.getZoom() > drawBondsZoom) {
        bondBatch.clear();
        bondBatch.reserve(Bond::bonds_list.size() * 2);

        for (const Bond& bond : Bond::bonds_list) {
            bondBatch.emplace_back(
                sf::Vector2f(
                    static_cast<float>(bond.a->coords.x + bond.a->getProps().radius - (bond.a->coords.z * alpha)) + boxOffset.x,
                    static_cast<float>(bond.a->coords.y + bond.a->getProps().radius - (bond.a->coords.z * alpha)) + boxOffset.y
                ),
                sf::Color::Blue
            );
            bondBatch.emplace_back(
                sf::Vector2f(
                    static_cast<float>(bond.b->coords.x + bond.b->getProps().radius - (bond.b->coords.z * alpha)) + boxOffset.x,
                    static_cast<float>(bond.b->coords.y + bond.b->getProps().radius - (bond.b->coords.z * alpha)) + boxOffset.y
                ),
                sf::Color::Blue
            );
        }

        if (!bondBatch.empty()) {
            window.draw(bondBatch.data(), bondBatch.size(), sf::PrimitiveType::Lines);
        }
    }

    if (drawSelectionFrame) window.draw(frameShape);

    window.setView(uiView);
    ImGui::SFML::Render(window);
    window.display();
}

void Renderer2D::drawTransparencyMap(sf::RenderWindow& window, const SpatialGrid& grid) {
    sf::RectangleShape cellRect(sf::Vector2f(
        static_cast<float>(grid.cellSize),
        static_cast<float>(grid.cellSize)
    ));
    cellRect.setFillColor(sf::Color(255, 0, 0, 120));
    cellRect.setOutlineColor(sf::Color(120, 0, 0, 180));
    cellRect.setOutlineThickness(-1.0f);

    for (int z = 0; z < grid.sizeZ; ++z) {
        for (int y = 0; y < grid.sizeY; ++y) {
            for (int x = 0; x < grid.sizeX; ++x) {
                if (auto cell = grid.at(x, y, z); cell && !cell->empty()) {
                    cellRect.setPosition({
                        static_cast<float>(x * grid.cellSize),
                        static_cast<float>(y * grid.cellSize)
                    });
                    window.draw(cellRect);
                }
            }
        }
    }
}

void Renderer2D::drawForceField(const sf::Texture& forceTexture, const SimBox& box) {
    if (!forceFieldShaderLoaded || forceTexture.getSize().x == 0 || forceTexture.getSize().y == 0) {
        return;
    }

    forceFieldShader.setUniform("field", forceTexture);
    forceFieldQuad.setSize(box.end - box.start);
    forceFieldQuad.setPosition(box.start);
    forceFieldQuad.setTexture(&forceTexture, true);

    window.draw(forceFieldQuad, &forceFieldShader);
}

void Renderer2D::setSelectionFrame(Vec2D start, Vec2D end, float scale) {
    frameShape.setPosition(start);
    frameShape.setSize(end - start);

    frameShape.setFillColor(sf::Color::Transparent);
    frameShape.setOutlineColor(sf::Color(60, 60, 60));
    frameShape.setOutlineThickness(1.f / scale);
}
