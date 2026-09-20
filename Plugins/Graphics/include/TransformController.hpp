#pragma once

#include "SceneObject.hpp"
#include "Lattice/Kernel/Component.hpp"
#include "Lattice/Kernel/Focus.hpp"
#include "Lattice/Kernel/Node.hpp"

class TransformController : public Lattice::Component {
public:
    void configure(Lattice::Node& node) {
        camera = node.focus<SceneObject>("camera");
        node.on("left", [this]() {
            if (camera)
                camera->move({-0.1, 0, 0});
        });

        node.on("right", [this]() {
            if (camera)
                camera->move({0.1, 0, 0});
        });

        // node.on("rotate",  [this]() {
        //     camera->rotate(delta);
        // });
    }

    Lattice::Focus<SceneObject> camera;
};