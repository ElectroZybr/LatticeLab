#pragma once

#include <limits>
#include <string>
#include <unordered_map>

#include <Lattice/Tools/Exception.hpp>

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

    template<typename Name>
    Id add(const Name& name, Id id, bool overwrite = false) {
        Key key{name};
        if (overwrite) {
            lookup_.insert_or_assign(std::move(key), id);
        } else if (!lookup_.emplace(std::move(key), id).second) {
            throw Lattice::Exception<NamedRegistry>("Name already exists");
        }
        return id;
    }

    template<typename Name>
    void alias(Id id, const Name& name, bool overwrite = false) {
        add(name, id, overwrite);
    }

    template<typename Name>
    void remove(const Name& name) {
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

    template<typename Name>
    Id find(const Name& name) const {
        const auto it = lookup_.find(Key{name});

        if (it == lookup_.end())
            return InvalidId;

        return it->second;
    }

    template<typename Name>
    bool has(const Name& name) const {
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