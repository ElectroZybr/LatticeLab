#pragma once

#include "TransformMode.hpp"
#include "TransformMode.hpp"
#include <glm/ext/vector_float2.hpp>
#include "Lattice/Kernel/Component.hpp"
#include "Lattice/Kernel/Focus.hpp"
#include "Lattice/Kernel/Node.hpp"
#include "Lattice/Tools/Logger.hpp"

class TransformController : public Lattice::Component {
public:
    void configure(Lattice::Node& node) {
        mode_ = node.slot<TransformMode>();
        target_ = node.focus<SceneObject>("Camera");

        // node.on("move", [this](glm::vec3 v) {
        //     if (mode_) mode_->move(target_, v);
        // });

        node.bind("look", &v, [this](glm::vec2 delta) {
            Logger::info("lambda", "{} {}", v.x, v.y);
            target_->move({v.x * -0.001, v.y * 0.001, 0});
        });

        // node.on("zoom", [this](float v) {
        //     if (mode_) mode_->zoom(target_, v);
        // });
    }

private:
    glm::vec2 v;
    Slot<TransformMode> mode_;
    Lattice::Focus<SceneObject> target_;
};