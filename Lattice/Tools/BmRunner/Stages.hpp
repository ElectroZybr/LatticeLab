#pragma once

#include <chrono>
#include <concepts>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <Lattice/Tools/BmRunner/BenchTypes.hpp>

namespace Lattice::Benchmarks {

/**
 @file Stages.hpp
 @brief Группировка измерительных capability по стадиям бенчмарка.

 Stages управляет наборами Capability, выполняемыми в рамках одного прохода
 теста и предназначенными для совместного сбора метрик.
*/

class Capability {
public:
    virtual ~Capability() = default;
    virtual std::string_view name() const noexcept = 0;

    virtual void begin() {} // Подготовить capability к sample: открыть/сбросить/инициализировать счётчики
    virtual void start() {} // Начать измеряемый участок, максимально дешёвая операция
    virtual void stop() {}  // Закончить измеряемый участок, максимально дешёвая операция
    virtual Metrics end() { return {}; }    // завершить sample: остановить/закрыть временное состояние, сформировать Metrics одного sample
    virtual Metrics result() { return {}; } // сформировать Metrics всего stage
};


class Stages {
public:
    struct Stage {
        std::vector<std::unique_ptr<Capability>> capabilities;
        size_t sampleLimit = 0;
        std::chrono::nanoseconds timeLimit{0};

        Stage& samples(size_t value) {
            sampleLimit = value;
            return *this;
        }

        template<typename Rep, typename Period>
        Stage& time(std::chrono::duration<Rep, Period> value) {
            timeLimit = std::chrono::duration_cast<std::chrono::nanoseconds>(value);
            return *this;
        }
    };

    template<typename... T>
    Stage& add() {
        static_assert(sizeof...(T) > 0);
        static_assert((std::derived_from<T, Capability> && ...));

        Stage stage;
        stage.capabilities.reserve(sizeof...(T));
        (stage.capabilities.push_back(std::make_unique<T>()), ...);

        stages_.push_back(std::move(stage));
        return stages_.back();
    }

    template<typename... T>
    Stage& add(T&&... capabilities) {
        static_assert(sizeof...(T) > 0);
        static_assert((std::derived_from<std::decay_t<T>, Capability> && ...));

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
        return stages_.back();
    }

    void clear() {
        stages_.clear();
    }

    bool empty() const noexcept {
        return stages_.empty();
    }

    std::vector<Stage>& data() noexcept {
        return stages_;
    }

    const std::vector<Stage>& data() const noexcept {
        return stages_;
    }

private:
    std::vector<Stage> stages_;
};

}