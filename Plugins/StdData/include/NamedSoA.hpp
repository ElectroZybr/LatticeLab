#pragma once

#include <cstddef>
#include <string_view>
#include <utility>

#include <Lattice/Tools/NamedRegistry.hpp>
#include "SoA.hpp"

namespace StdData {

/**
 @brief Табличный контейнер с идентификацией строк по имени.

 Объединяет SoA с реестром имён, позволяя находить строки по имени,
 добавлять новые записи и дополнять существующие данные по ключу.
*/

class NamedSoA : public SoA {
public:
    NamedSoA() = default;
    explicit NamedSoA(NodeBuild branch)
        : SoA(std::move(branch)) {}

    void clear() noexcept {
        SoA::clear();
        names_.clear();
    }

    [[nodiscard]] size_t find(std::string_view name) const noexcept {
        return names_.find(name);
    }

    [[nodiscard]] bool hasRow(std::string_view name) const noexcept {
        return names_.has(name);
    }

    template<typename... Values>
    [[nodiscard]] size_t addRow(std::string_view name, Values&&... values) {
        const size_t id = sizeof...(Values) == 0
            ? SoA::addRow()
            : SoA::addRow(std::forward<Values>(values)...);
        names_.add(name, id);
        return id;
    }

    [[nodiscard]] size_t requireRow(std::string_view name) const {
        const size_t id = find(name);

        if (!Lattice::NamedRegistry<size_t>::valid(id))
            throw Lattice::Exception("NamedSoA", "Row '{}' not found", name);

        return id;
    }

private:
    Lattice::NamedRegistry<size_t> names_;
};

} // namespace StdData
