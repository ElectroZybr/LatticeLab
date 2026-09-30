#pragma once

#include <string_view>
#include <vector>

#include <Lattice/Tools/BmRunner/Bench.hpp>
#include <Lattice/Tools/BmRunner/BenchTypes.hpp>
#include <Lattice/Tools/Fixture.hpp>


/**
 @file Benchmarks.hpp
 @brief Регистрация и запуск бенчмарков.

 Предоставляет макросы автоматической регистрации бенчмарков, групповых
 конфигураций, функции запуска зарегистрированных тестов, получения их списка
 и настройки callback-ов для обработки результатов выполнения.
*/


#define BENCH2(name, FixtureType) \
    BENCH3(name, FixtureType, "")

#define BENCH3(name, FixtureType, description) \
    static void name(::Lattice::Benchmarks::Bench& bench); \
    static std::unique_ptr<::Lattice::Fixture> _fixture_##name(size_t n) { \
        return std::make_unique<FixtureType>(n); \
    } \
    static ::Lattice::Benchmarks::Registrar _bench_##name( \
        "", \
        #name, \
        description, \
        name, \
        _fixture_##name \
    ); \
    static void name(::Lattice::Benchmarks::Bench& bench)

#define BENCH_SELECT(_1, _2, _3, NAME, ...) NAME
#define BENCH(...) BENCH_SELECT(__VA_ARGS__, BENCH3, BENCH2)(__VA_ARGS__)
    

#define BENCH_GROUPED3(group, name, FixtureType) \
    BENCH_GROUPED4(group, name, FixtureType, "")

#define BENCH_GROUPED4(group, name, FixtureType, description) \
    static void name(::Lattice::Benchmarks::Bench& bench); \
    static std::unique_ptr<::Lattice::Fixture> _fixture_##name(size_t n) { \
        return std::make_unique<FixtureType>(n); \
    } \
    static ::Lattice::Benchmarks::Registrar _bench_##name( \
        #group, \
        #name, \
        description, \
        name, \
        _fixture_##name \
    ); \
    static void name(::Lattice::Benchmarks::Bench& bench)

#define BENCH_GROUPED_SELECT(_1, _2, _3, _4, NAME, ...) NAME
#define BENCH_GROUPED(...) \
    BENCH_GROUPED_SELECT(__VA_ARGS__, BENCH_GROUPED4, BENCH_GROUPED3)(__VA_ARGS__)


#define BENCH_GROUP(group) \
    static void _bench_group_config_##group(::Lattice::Benchmarks::Bench& bench); \
    static ::Lattice::Benchmarks::GroupRegistrar _bench_group_registrar_##group( \
        #group, \
        _bench_group_config_##group \
    ); \
    static void _bench_group_config_##group(::Lattice::Benchmarks::Bench& bench)


namespace Lattice::Benchmarks {

struct Info {
    std::string_view group;
    std::string_view name;
    std::string_view description;
};

class Registrar {
public:
    Registrar(
        std::string_view group,
        std::string_view name,
        std::string_view description,
        void (*function)(Bench&),
        Fixture::Factory createFixture
    );
};

class GroupRegistrar {
public:
    GroupRegistrar(
        std::string_view group,
        void (*function)(Bench&)
    );
};

void run(std::string_view name);
void runGroup(std::string_view group);
void runAll();

std::vector<Info> list();
std::vector<std::string_view> groups();

void setSampleCallback(SampleCallback callback);
void setResultCallback(ResultCallback callback);
void setCompleteCallback(CompleteCallback callback);

void disableSampleCallback();
void disableResultCallback();
void disableCompleteCallback();
void disableCallbacks();

}