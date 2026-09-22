#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/Ids.hpp>
#include <Lattice/Kernel/ObjectRegistry.hpp>

namespace Lattice {

class NodeBuildView;
class NodeConfigureView;

struct BlueprintMeta {
    std::string_view descriptor;
    void* (*create)(NodeBuildView, const void*) = nullptr;
    void (*configure)(void*, NodeConfigureView) = nullptr;
    void (*destroy)(void*) = nullptr;
};

struct Blueprint {
    std::string name;
    std::vector<BlueprintId> bases;
    bool exists = true;
    BlueprintMeta meta;
    std::vector<void* (*)(void*)> upcasts;

    std::string_view shortName() const noexcept;
    std::string_view namespaceName() const noexcept;
};

class Blueprints : public ObjectRegistry<Blueprint, BlueprintId, std::string> {
    using Base = ObjectRegistry<Blueprint, BlueprintId, std::string>;

public:
    BlueprintId add(
        std::string_view name,
        std::initializer_list<BlueprintId> bases = {}
    );

    BlueprintId resolveImplementation(
        BlueprintId api,
        std::string_view nameSpace = {},
        std::string_view descriptor = {}
    ) const;

    void* cast(BlueprintId from, BlueprintId to, void* object) const;

    BlueprintId find(
        std::string_view name,
        std::string_view nameSpace = {}
    ) const;

    BlueprintId resolve(std::string_view name) const;

    void addBase(BlueprintId derived, BlueprintId base);

    bool isA(BlueprintId derived, BlueprintId base) const;

    std::vector<BlueprintId> getImpls(
        BlueprintId api,
        bool constructibleOnly = true
    ) const;

    void dumpTree() const;

private:
    static void validateName(std::string_view name);
};

}