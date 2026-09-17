#pragma once

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/ObjectRegistry.hpp>

namespace Lattice {

using BlueprintId = uint32_t;

struct Blueprint {
    std::string name; // Полное имя, например WGPU::Device.
    std::vector<BlueprintId> bases;
    bool exists = true;

    std::string_view shortName() const noexcept {
        const auto pos = name.rfind("::");
        return std::string_view(name).substr(pos == std::string::npos ? 0 : pos + 2);
    }

    std::string_view namespaceName() const noexcept {
        const auto pos = name.rfind("::");
        return pos == std::string::npos ? std::string_view{} : std::string_view(name).substr(0, pos);
    }
};

// Отдельный реестр типов. ID не относятся к Objects и не переиспользуются.
// Наружу выдаётся только const-доступ: рёбра изменяются через проверку циклов.
class Blueprints {
    using Registry = ObjectRegistry<Blueprint, BlueprintId, std::string>;
public:
    static constexpr BlueprintId InvalidId = Registry::InvalidId;

    BlueprintId add(std::string_view name, std::initializer_list<BlueprintId> bases = {}) {
        validateName(name);
        std::vector<BlueprintId> uniqueBases;
        for (const auto base : bases) {
            require(base);
            if (std::find(uniqueBases.begin(), uniqueBases.end(), base) == uniqueBases.end())
                uniqueBases.push_back(base);
        }
        return registry_.create({std::string(name), std::move(uniqueBases)});
    }

    // Имя внутри namespace: find("Device", "WGPU") == find("WGPU::Device").
    // Поиск точный, без неявного перехода в соседние namespace.
    BlueprintId find(std::string_view name, std::string_view nameSpace = {}) const {
        if (nameSpace.empty())
            return registry_.find(name);
        return registry_.find(std::string(nameSpace) + "::" + std::string(name));
    }

    // Короткое имя допустимо только при единственном совпадении во всём реестре.
    BlueprintId resolve(std::string_view name) const {
        if (name.find("::") != std::string_view::npos)
            return find(name);

        BlueprintId result = InvalidId;
        for (BlueprintId id = 0; id < size(); ++id) {
            const auto& blueprint = require(id);
            if (blueprint.shortName() != name)
                continue;
            if (result != InvalidId)
                throw Exception("Blueprints", "Ambiguous name '{}': '{}' and '{}'", name, require(result).name, blueprint.name);
            result = id;
        }
        return result;
    }

    const Blueprint* get(BlueprintId id) const { return registry_.get(id); }
    const Blueprint& require(BlueprintId id) const { return registry_.require(id); }
    BlueprintId size() const { return registry_.size(); }

    void addBase(BlueprintId derived, BlueprintId base) {
        require(derived);
        require(base);
        if (isA(base, derived))
            throw Exception("Blueprints", "Inheritance cycle: '{}' -> '{}'", require(derived).name, require(base).name);

        auto& bases = registry_.get(derived)->bases;
        if (std::find(bases.begin(), bases.end(), base) == bases.end())
            bases.push_back(base);
    }

    // Рефлексивное и транзитивное отношение: тип является самим собой.
    // Это проверка графа, не гарантия однозначного C++-приведения указателя.
    bool isA(BlueprintId derived, BlueprintId base) const {
        if (!get(derived) || !get(base))
            return false;

        std::vector<bool> visited(size(), false);
        std::vector<BlueprintId> pending{derived};
        while (!pending.empty()) {
            const auto id = pending.back();
            pending.pop_back();
            if (id == base)
                return true;
            if (visited[id])
                continue;
            visited[id] = true;
            const auto& bases = require(id).bases;
            pending.insert(pending.end(), bases.begin(), bases.end());
        }
        return false;
    }

private:
    static void validateName(std::string_view name) {
        if (name.empty())
            throw Exception("Blueprints", "Blueprint name must not be empty");
        size_t begin = 0;
        while (begin < name.size()) {
            auto end = name.find("::", begin);
            if (end == std::string_view::npos)
                end = name.size();
            const auto part = name.substr(begin, end - begin);
            if (part.empty() || part.find_first_of(": \t\r\n") != std::string_view::npos)
                throw Exception("Blueprints", "Invalid qualified name '{}'", name);
            if (end == name.size())
                return;
            begin = end + 2;
        }
        throw Exception("Blueprints", "Invalid qualified name '{}'", name);
    }

    Registry registry_;
};

} // namespace Lattice
