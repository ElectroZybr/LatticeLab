#include "Benchmarks.hpp"

#include <algorithm>
#include <utility>

#include <Lattice/Tools/BmRunner/Output.hpp>
#include <Lattice/Tools/ObjectRegistry.hpp>
#include <Lattice/Tools/Exception.hpp>

namespace Lattice::Benchmarks {

namespace {

using BenchId = uint32_t;

struct Registry {};

struct Case {
    std::string group;
    std::string name;
    std::string description;
    void (*function)(Bench&);
    Fixture::Factory createFixture;
};

struct GroupConfig {
    std::string name;
    void (*function)(Bench&);
};

struct State {
    ObjectRegistry<Case, BenchId, std::string> benches;
    std::vector<GroupConfig> groups;

    SampleCallback sample = Output::sample;
    ResultCallback result = Output::result;
    CompleteCallback complete = Output::complete;
};

State& state() {
    static State value;
    return value;
}

void add(Case bench) {
    state().benches.create(std::move(bench));
}

void addGroupConfig(GroupConfig config) {
    auto& groups = state().groups;

    if (std::ranges::find(groups, config.name, &GroupConfig::name) != groups.end())
        throw Exception<Registry>("Benchmark group '{}' is already configured", config.name);

    groups.push_back(std::move(config));
}

const GroupConfig* findGroupConfig(std::string_view group) {
    auto& groups = state().groups;
    const auto it = std::ranges::find(groups, group, &GroupConfig::name);
    return it != groups.end() ? &*it : nullptr;
}

void execute(const Case& benchCase) {
    State& s = state();

    Bench bench(
        benchCase.group,
        benchCase.name,
        benchCase.createFixture,
        s.sample,
        s.result,
        s.complete
    );

    if (const GroupConfig* config = findGroupConfig(benchCase.group))
        config->function(bench);

    benchCase.function(bench);
}

}

Registrar::Registrar(
    std::string_view group,
    std::string_view name,
    std::string_view description,
    void (*function)(Bench&),
    Fixture::Factory createFixture
) {
    add({
        std::string(group),
        std::string(name),
        std::string(description),
        function,
        createFixture
    });
}

GroupRegistrar::GroupRegistrar(std::string_view group, void (*function)(Bench&)) {
    addGroupConfig({
        .name = std::string(group),
        .function = function
    });
}

void run(std::string_view name) {
    State& s = state();
    const BenchId id = s.benches.find(name);

    if (!s.benches.valid(id))
        throw Exception<Registry>("Benchmark '{}' not found", name);

    execute(s.benches.require(id));
}

void runGroup(std::string_view group) {
    State& s = state();

    for (BenchId id = 0; id < s.benches.size(); ++id) {
        const Case* bench = s.benches.get(id);

        if (bench && bench->group == group)
            execute(*bench);
    }
}

void runAll() {
    State& s = state();

    for (BenchId id = 0; id < s.benches.size(); ++id)
        if (const Case* bench = s.benches.get(id))
            execute(*bench);
}

std::vector<Info> list() {
    State& s = state();
    std::vector<Info> result;

    for (BenchId id = 0; id < s.benches.size(); ++id) {
        const Case* bench = s.benches.get(id);
        if (!bench)
            continue;

        result.push_back({
            .group = bench->group,
            .name = bench->name,
            .description = bench->description
        });
    }

    return result;
}

std::vector<std::string_view> groups() {
    State& s = state();
    std::vector<std::string_view> result;

    for (BenchId id = 0; id < s.benches.size(); ++id) {
        const Case* bench = s.benches.get(id);

        if (!bench || bench->group.empty())
            continue;

        if (std::ranges::find(result, bench->group) == result.end())
            result.push_back(bench->group);
    }

    return result;
}

void setSampleCallback(SampleCallback callback) {
    state().sample = std::move(callback);
}

void setResultCallback(ResultCallback callback) {
    state().result = std::move(callback);
}

void setCompleteCallback(CompleteCallback callback) {
    state().complete = std::move(callback);
}

void disableSampleCallback() {
    state().sample = {};
}

void disableResultCallback() {
    state().result = {};
}

void disableCompleteCallback() {
    state().complete = {};
}

void disableCallbacks() {
    State& s = state();
    s.sample = {};
    s.result = {};
    s.complete = {};
}

}