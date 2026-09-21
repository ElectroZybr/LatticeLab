#pragma once

#include <glm/glm.hpp>
#include "SceneObject.hpp"
#include "Lattice/Kernel/Component.hpp"

class TransformMode : public Lattice::Component {
public:
    virtual void move(SceneObject& target, glm::vec3 delta) = 0;
    virtual void look(SceneObject& target, glm::vec2 delta) = 0;
    virtual void zoom(SceneObject& target, float delta) = 0;
};

class FreeTransform : public TransformMode {
public:
    void move(SceneObject& target, glm::vec3 delta) {

    }

    void look(SceneObject& target, glm::vec2 delta) {

    }

    void zoom(SceneObject& target, float delta) {

    }
};