#pragma once

#include <Lattice/Kernel/Component.hpp>


struct SubsystemAPI : public Lattice::Component {
    SubsystemAPI() = default;
    SubsystemAPI(const SubsystemAPI&) = delete;
    SubsystemAPI& operator=(const SubsystemAPI&) = delete;
    SubsystemAPI& operator=(SubsystemAPI&&) = delete;
};