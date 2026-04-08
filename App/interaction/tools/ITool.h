#pragma once

#include <functional>
#include <memory>

#include <SFML/Graphics.hpp>

#include "Engine/math/Vec3f.h"
#include "Engine/physics/AtomData.h"

class AtomStorage;
class IRenderer;
class PickingSystem;
class SimBox;
class SpatialGrid;
struct UiState;

struct ToolContext {
    using AtomCreator = std::function<bool(Vec3f, Vec3f, AtomData::Type, bool)>;
    using AtomRemover = std::function<bool(size_t)>;

    sf::RenderWindow* window = nullptr;
    sf::View* gameView = nullptr;
    SpatialGrid* grid = nullptr;
    SimBox* box = nullptr;
    std::unique_ptr<IRenderer>* renderer = nullptr;
    AtomStorage* atomStorage = nullptr;
    PickingSystem* pickingSystem = nullptr;
    UiState* uiState = nullptr;
    AtomCreator atomCreator{};
    AtomRemover atomRemover{};

    [[nodiscard]] bool isValid() const noexcept {
        return window != nullptr && gameView != nullptr && box != nullptr && renderer != nullptr && atomStorage != nullptr;
    }

    [[nodiscard]] IRenderer* activeRenderer() const noexcept { return (renderer != nullptr) ? renderer->get() : nullptr; }
};

class ITool {
public:
    explicit ITool(ToolContext& context) noexcept;
    virtual ~ITool();

    ITool(const ITool&) = delete;
    ITool& operator=(const ITool&) = delete;

    virtual void onLeftPressed(sf::Vector2i mousePos);
    virtual void onLeftReleased(sf::Vector2i mousePos);
    virtual bool onRightPressed(sf::Vector2i mousePos);
    virtual void onFrame(sf::Vector2i mousePos, float deltaTime);
    virtual void reset();

protected:
    [[nodiscard]] ToolContext& context() noexcept { return context_; }
    [[nodiscard]] const ToolContext& context() const noexcept { return context_; }

    [[nodiscard]] Vec3f screenToWorld(sf::Vector2i mousePos) const;
    [[nodiscard]] sf::Vector2i worldToScreen(Vec3f worldPos) const;

private:
    ToolContext& context_;
};
