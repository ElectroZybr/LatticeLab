#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <Lattice/Tools/BmRunner/Stage.hpp>

namespace Lattice {

class Stages {
public:
    struct Stage {
        std::vector<std::unique_ptr<StageCapability>> capabilities;
    };

private:
    std::vector<Stage> stages_;

public:
    template<typename... T>
    Stages& add() {
        Stage stage;
        stage.capabilities.reserve(sizeof...(T));

        (stage.capabilities.push_back(std::make_unique<T>()), ...);

        stages_.push_back(std::move(stage));
        return *this;
    }

    template<typename... T>
    Stages& add(T&&... capabilities) {
        Stage stage;
        stage.capabilities.reserve(sizeof...(T));

        (
            stage.capabilities.push_back(
                std::make_unique<std::decay_t<T>>(
                    std::forward<T>(capabilities)
                )
            ),
            ...
        );

        stages_.push_back(std::move(stage));
        return *this;
    }

    void clear() {
        stages_.clear();
    }

    bool empty() const noexcept {
        return stages_.empty();
    }

    auto& data() noexcept {
        return stages_;
    }

    const auto& data() const noexcept {
        return stages_;
    }
};

}