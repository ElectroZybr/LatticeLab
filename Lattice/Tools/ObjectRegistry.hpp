#pragma once

#include <optional>
#include <vector>

#include <Lattice/Tools/Exception.hpp>
#include <Lattice/Tools/NamedRegistry.hpp>

namespace Lattice {

/**
 @file ObjectRegistry.hpp
 @brief Хранилище объектов с идентификацией по имени.

 ObjectRegistry предоставляет операции для создания, поиска, получения,
 удаления и управления объектами, связанными с идентификаторами.

 Для сопоставления имён с идентификаторами используется NamedRegistry.
*/

template<typename Object, typename Id, typename Key, typename Hash = std::hash<Key>>
class ObjectRegistry {
public:
    Id create(Object object) {
        const Key key{object.name};
        return create(std::move(object), key);
    }

    Id create(Object object, std::optional<Key> key, bool overwrite = false) {
        const Id id = freeIds_.empty() ? static_cast<Id>(objects_.size()) : freeIds_.back();

        if (key)
            names_.add(*key, id, overwrite);

        if (!freeIds_.empty()) {
            freeIds_.pop_back();
            objects_[id].emplace(std::move(object));
        } else {
            objects_.emplace_back(std::move(object));
        }

        return id;
    }

    // добавляет псевдоним к слоту
    template<typename Name>
    void alias(Id id, const Name& name, bool overwrite = false) {
        if (!get(id))
            return;

        names_.alias(id, name, overwrite);
    }

    // уничтожает слот
    void destroy(Id id) {
        if (!get(id))
            return;

        names_.remove(id);
        objects_[id].reset();
        freeIds_.push_back(id);
    }

    // строгое требование наличия объекта
    const Object& require(Id id) const {
        const Object* object = get(id);

        if (!object)
            throw Lattice::Exception("Registry", "Object with id {} not found", id);

        return *object;
    }

    Object& require(Id id) {
        Object* object = get(id);

        if (!object)
            throw Lattice::Exception("Registry", "Object with id {} not found", id);

        return *object;
    }

    // доступ по индексу, nullptr если не найден
    const Object* get(Id id) const {
        if (!valid(id) || id >= objects_.size() || !objects_[id])
            return nullptr;

        return &*objects_[id];
    }

    Object* get(Id id) {
        if (!valid(id) || id >= objects_.size() || !objects_[id])
            return nullptr;

        return &*objects_[id];
    }

    // ищет id в реестре по строковому имени
    template<typename Name>
    Id find(const Name& name) const {
        return names_.find(name);
    }

    // проверяет наличие имени в реестре
    template<typename Name>
    bool has(const Name& name) const {
        return names_.has(name);
    }

    void clear() {
        objects_.clear();
        freeIds_.clear();
        names_.clear();
    }

    Id size() const {
        return static_cast<Id>(objects_.size());
    }

    static constexpr Id InvalidId = NamedRegistry<Id, Key, Hash>::InvalidId;

    static constexpr bool valid(Id id) noexcept {
        return NamedRegistry<Id, Key, Hash>::valid(id);
    }

private:
    std::vector<std::optional<Object>> objects_;
    std::vector<Id> freeIds_;
    NamedRegistry<Id, Key, Hash> names_;
};

}
