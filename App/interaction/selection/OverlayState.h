#pragma once

#include <cmath>
#include <numbers>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>

struct OverlayState {
    bool boxVisible = false;
    bool lassoVisible = false;
    bool rulerVisible = false;

    sf::Vector2i boxStart;
    sf::Vector2i boxEnd;
    sf::Vector2i rulerStart;
    sf::Vector2i rulerEnd;
    std::string rulerLabel;

    std::vector<sf::Vector2i> lassoPoints;

    void reset() {
        boxVisible = false;
        lassoVisible = false;
        rulerVisible = false;
        rulerLabel.clear();
        lassoPoints.clear();
    }

    void draw(sf::RenderTarget& target) const {
        if (boxVisible || lassoVisible || rulerVisible) {
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

            for (size_t i = 0; i < 5; ++i) {
                box[i].color = sf::Color::Red;
            }

            target.draw(box);
        }
        if (lassoVisible && !lassoPoints.empty()) {
            sf::VertexArray lasso(sf::PrimitiveType::LineStrip, lassoPoints.size() + 1);
            for (size_t i = 0; i < lassoPoints.size(); ++i) {
                lasso[i].position = sf::Vector2f(lassoPoints[i].x, lassoPoints[i].y);
                lasso[i].color = sf::Color::Red;
            }
            lasso[lassoPoints.size()] = lasso[0];
            target.draw(lasso);
        }
        if (rulerVisible) {
            sf::VertexArray ruler(sf::PrimitiveType::Lines, 2);
            ruler[0].position = sf::Vector2f(rulerStart.x, rulerStart.y);
            ruler[1].position = sf::Vector2f(rulerEnd.x, rulerEnd.y);
            ruler[0].color = sf::Color(255, 90, 90);
            ruler[1].color = sf::Color(255, 90, 90);
            target.draw(ruler);

            sf::CircleShape startPoint(4.0f);
            startPoint.setOrigin(sf::Vector2f(4.0f, 4.0f));
            startPoint.setPosition(sf::Vector2f(rulerStart.x, rulerStart.y));
            startPoint.setFillColor(sf::Color(255, 90, 90));
            target.draw(startPoint);

            sf::CircleShape endPoint(4.0f);
            endPoint.setOrigin(sf::Vector2f(4.0f, 4.0f));
            endPoint.setPosition(sf::Vector2f(rulerEnd.x, rulerEnd.y));
            endPoint.setFillColor(sf::Color(255, 90, 90));
            target.draw(endPoint);

            if (!rulerLabel.empty()) {
                static sf::Font rulerFont;
                static bool fontInitialized = false;
                static bool fontAvailable = false;
                if (!fontInitialized) {
                    fontAvailable = rulerFont.openFromFile("assets/fonts/Rubik-VariableFont_wght.ttf");
                    fontInitialized = true;
                }

                if (fontAvailable) {
                    const sf::Vector2f startPos(static_cast<float>(rulerStart.x), static_cast<float>(rulerStart.y));
                    const sf::Vector2f endPos(static_cast<float>(rulerEnd.x), static_cast<float>(rulerEnd.y));
                    const sf::Vector2f line = endPos - startPos;
                    const float lineLength = std::sqrt(line.x * line.x + line.y * line.y);

                    sf::Vector2f labelPos = 0.5f * (startPos + endPos);
                    float angleDeg = 0.0f;
                    if (lineLength > 0.001f) {
                        sf::Vector2f normal(-line.y / lineLength, line.x / lineLength);
                        if (normal.y > 0.0f) {
                            normal = -normal;
                        }

                        constexpr float kLabelOffset = 16.0f;
                        labelPos += normal * kLabelOffset;

                        angleDeg = std::atan2(line.y, line.x) * 180.0f / std::numbers::pi;
                        if (angleDeg > 90.0f) {
                            angleDeg -= 180.0f;
                        }
                        else if (angleDeg < -90.0f) {
                            angleDeg += 180.0f;
                        }
                    }
                    else {
                        labelPos.y -= 10.0f;
                    }

                    labelPos.x = std::round(labelPos.x);
                    labelPos.y = std::round(labelPos.y);

                    sf::Text label(rulerFont, rulerLabel, 18);
                    label.setFillColor(sf::Color(235, 235, 235));
                    const sf::FloatRect bounds = label.getLocalBounds();
                    label.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f));
                    label.setPosition(labelPos);
                    label.setRotation(sf::degrees(angleDeg));
                    target.draw(label);
                }
            }
        }

        if (boxVisible || lassoVisible || rulerVisible) {
            target.popGLStates();
        }
    }
};
