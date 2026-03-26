#pragma once

#include <vector>
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>

struct OverlayState {
    bool boxVisible   = false;
    bool lassoVisible = false;

    sf::Vector2i boxStart;
    sf::Vector2i boxEnd;

    std::vector<sf::Vector2i> lassoPoints;

    void reset() {
        boxVisible   = false;
        lassoVisible = false;
        lassoPoints.clear();
    }

    void draw(sf::RenderTarget& target) const
    {
        if (boxVisible || lassoVisible)
        {
            target.pushGLStates();
            target.setView(target.getDefaultView());
        }
        
        if (boxVisible) {
            sf::VertexArray box(sf::PrimitiveType::LineStrip, 5);
            box[0].position = sf::Vector2f(boxStart.x, boxStart.y);
            box[1].position = sf::Vector2f(boxEnd.x, boxStart.y);
            box[2].position = sf::Vector2f(boxEnd.x, boxEnd.y);
            box[3].position = sf::Vector2f(boxStart.x, boxEnd.y);
            box[4].position = box[0].position;

            for (size_t i = 0; i < 5; ++i) box[i].color = sf::Color::Red;

            target.draw(box);
        }
        if (lassoVisible && !lassoPoints.empty()) {
            sf::VertexArray lasso(sf::PrimitiveType::LineStrip, lassoPoints.size()+1);
            for (size_t i = 0; i < lassoPoints.size(); ++i) {
                lasso[i].position = sf::Vector2f(lassoPoints[i].x, lassoPoints[i].y);
                lasso[i].color = sf::Color::Red;
            }
            lasso[lassoPoints.size()] = lasso[0];
            target.draw(lasso);
        }

        if (boxVisible || lassoVisible)
            target.popGLStates();
    }
};
