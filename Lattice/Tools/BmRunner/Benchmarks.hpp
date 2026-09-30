#pragma once

#include <string_view>
#include <vector>

#include <Lattice/Tools/BmRunner/Bench.hpp>
#include <Lattice/Tools/BmRunner/BenchTypes.hpp>
#include <Lattice/Tools/ObjectRegistry.hpp>


/**
 @file Benchmarks.hpp
 @brief Регистрация и запуск бенчмарков.

 Предоставляет макросы автоматической регистрации бенчмарков, групповых
 конфигураций, функции запуска зарегистрированных тестов, получения их списка
 и настройки callback-ов для обработки результатов выполнения.
*/


#define BENCH1(name) \
    static void name(::Lattice::Benchmarks::Bench& bench); \
    static ::Lattice::Benchmarks::Registrar _bench_##name("", #name, "", name); \
    static void name(::Lattice::Benchmarks::Bench& bench)

#define BENCH2(group, name) \
    static void name(::Lattice::Benchmarks::Bench& bench); \
    static ::Lattice::Benchmarks::Registrar _bench_##name(#group, #name, "", name); \
    static void name(::Lattice::Benchmarks::Bench& bench)

#define BENCH3(group, name, description) \
    static void name(::Lattice::Benchmarks::Bench& bench); \
    static ::Lattice::Benchmarks::Registrar _bench_##name(#group, #name, description, name); \
    static void name(::Lattice::Benchmarks::Bench& bench)

#define BENCH_SELECT(_1, _2, _3, NAME, ...) NAME
#define BENCH(...) BENCH_SELECT(__VA_ARGS__, BENCH3, BENCH2, BENCH1)(__VA_ARGS__)

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
        void (*function)(Bench&)
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