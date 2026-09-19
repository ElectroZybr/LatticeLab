// #pragma once

// #include "Lattice/Kernel/Component.hpp"
// #include "Lattice/Kernel/Node.hpp"

// class TransformController : public Lattice::Component {
// public:
//     void configure(Lattice::Node& node) {
//         node.bind("move", [this](glm::vec3 delta) {
//             object->move(delta);
//         });

//         node.bind("rotate", &node.requireContext()[0]., [this](glm::vec3 delta) {
//             object->rotate(delta);
//         });
//     }
// };