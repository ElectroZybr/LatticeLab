#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <concepts>
#include <memory>
#include <span>
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
    virtual bool available() const noexcept { return true; }
    virtual std::string_view unavailableReason() const noexcept { return {}; }

    virtual void begin() {} // Подготовить capability к sample: открыть/сбросить/инициализировать счётчики
    virtual void start() {} // Начать измеряемый участок, максимально дешёвая операция
    virtual void stop() {}  // Закончить измеряемый участок, максимально дешёвая операция
    virtual Metrics end() { return {}; }    // завершить sample: остановить/закрыть временное состояние, сформировать Metrics одного sample
    virtual Metrics result() { return {}; } // сформировать Metrics всего stage
};

template<size_t Size>
struct StaticString {
    char value[Size];

    constexpr StaticString(const char (&text)[Size]) {
        for (size_t i = 0; i < Size; ++i)
            value[i] = text[i];
    }

    constexpr operator std::string_view() const noexcept {
        return {value, Size - 1};
    }
};

template<StaticString Name>
class NamedCapability : public Capability {
public:
    static constexpr std::string_view capabilityName() noexcept {
        return Name;
    }

    std::string_view name() const noexcept final {
        return capabilityName();
    }
};

template<typename Owner>
struct MetricDefinition {
    std::string_view name;
    Unit unit = Unit::None;
    MetricFlags flags = MetricFlags::None;

    constexpr operator MetricDesc() const noexcept {
        return {name, unit, flags};
    }
};

template<StaticString Name, typename Derived>
class MetricCapability : public NamedCapability<Name> {
protected:
    static consteval MetricDefinition<Derived> defineMetric(
        std::string_view name,
        Unit unit,
        MetricFlags flags = MetricFlags::None
    ) {
        return {name, unit, flags};
    }

    template<typename... Definitions>
    static consteval auto defineSchema(Definitions... metrics) {
        static_assert(
            (
                std::same_as<
                    Definitions,
                    MetricDefinition<Derived>
                > && ...
            )
        );

        return std::array<MetricDesc, sizeof...(Definitions)>{
            static_cast<MetricDesc>(metrics)...
        };
    }

public:
    static constexpr std::span<const MetricDesc> schema() noexcept {
        return Derived::Schema;
    }
};

template<typename Owner>
constexpr ValueRef valueRef(MetricDefinition<Owner> definition) {
    const auto schema = Owner::schema();

    for (size_t i = 0; i < schema.size(); ++i) {
        if (schema[i].name == definition.name) {
            return {
                .source = ValueSource::Metric,
                .capability = Owner::capabilityName(),
                .name = definition.name,
                .unit = definition.unit,
                .index = i
            };
        }
    }

    throw "Metric is not part of its capability schema";
}

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
