#pragma once

#include <cstddef>
#include <string_view>

#include <Lattice/Kernel/NamedRegistry.hpp>
#include "SoA.hpp"

namespace StdData {

/**
 @brief Табличный контейнер с идентификацией строк по имени.

 Объединяет SoA с реестром имён, позволяя находить строки по имени,
 добавлять новые записи и дополнять существующие данные по ключу.
*/

class NamedSoA : public SoA {
public:
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

    [[nodiscard]] size_t addRow(std::string_view name) {
        const size_t id = size();
        names_.add(name, id);
        resize(id + 1);
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