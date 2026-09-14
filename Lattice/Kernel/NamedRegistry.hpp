#pragma once

#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>

#include <Lattice/Kernel/Exception.hpp>

namespace Lattice {

/**
 @file NamedRegistry.hpp
 @brief Сопоставление имён с идентификаторами.

 NamedRegistry предоставляет операции для регистрации, поиска,
 проверки, добавления псевдонимов и удаления имён.

 Реестр не владеет объектами и хранит только соответствие
 «имя → идентификатор».
*/

template<typename Id, typename Key = std::string, typename Hash = std::hash<Key>>
class NamedRegistry {
public:
    static constexpr Id InvalidId = std::numeric_limits<Id>::max();

    static constexpr bool valid(Id id) noexcept {
        return id != InvalidId;
    }

    Id add(std::string_view name, Id id) {
        const Key key{name};

        if (lookup_.contains(key))
            throw Lattice::Exception("NamedRegistry", "Name '{}' already exists", name);

        lookup_.emplace(std::move(key), id);
        return id;
    }

    void alias(Id id, std::string_view name) {
        add(name, id);
    }

    void remove(std::string_view name) {
        lookup_.erase(Key{name});
    }

    void remove(Id id) {
        for (auto it = lookup_.begin(); it != lookup_.end();) {
            if (it->second == id)
                it = lookup_.erase(it);
            else
                ++it;
        }
    }

    Id find(std::string_view name) const {
        const auto it = lookup_.find(Key{name});

        if (it == lookup_.end())
            return InvalidId;

        return it->second;
    }

    bool has(std::string_view name) const {
        return lookup_.contains(Key{name});
    }

    void clear() noexcept {
        lookup_.clear();
    }

    size_t size() const noexcept {
        return lookup_.size();
    }

    bool empty() const noexcept {
        return lookup_.empty();
    }

private:
    std::unordered_map<Key, Id, Hash> lookup_;
};

}