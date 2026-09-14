#pragma once

#include <string_view>
#include <vector>

#include <Lattice/Kernel/Exception.hpp>
#include "Lattice/Kernel/NamedRegistry.hpp"

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
        const Id id = allocate();

        names_.add(object.name, id);

        if (id == static_cast<Id>(objects_.size()))
            objects_.push_back(std::move(object));
        else
            objects_[id] = std::move(object);

        return id;
    }

    // добавляет псевдоним к слоту
    void alias(Id id, std::string_view name) {
        if (!get(id))
            return;

        names_.alias(id, name);
    }

    // уничтожает слот
    void destroy(Id id) {
        if (!get(id))
            return;

        names_.remove(id);

        objects_[id].exists = false;
        objects_[id].node = nullptr;
        objects_[id].name.clear();

        freeIds_.push_back(id);
    }

    // строгое требование наличия объекта
    const Object& require(Id id) const {
        const Object* object = get(id);

        if (!object)
            throw Lattice::Exception("Registry", "Object with id {} not found", id);

        return *object;
    }

    // доступ по индексу, nullptr если не найден
    const Object* get(Id id) const {
        if (!valid(id) || id >= objects_.size())
            return nullptr;

        const Object& object = objects_[id];

        if (!object.exists)
            return nullptr;

        return &object;
    }

    Object* get(Id id) {
        if (!valid(id) || id >= objects_.size())
            return nullptr;

        Object& object = objects_[id];

        if (!object.exists)
            return nullptr;

        return &object;
    }

    // ищет id в реестре по строковому имени
    Id find(std::string_view name) const {
        return names_.find(name);
    }

    // проверяет наличие имени в реестре
    bool has(std::string_view name) const {
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
    Id allocate() {
        if (!freeIds_.empty()) {
            const Id id = freeIds_.back();
            freeIds_.pop_back();
            return id;
        }

        return static_cast<Id>(objects_.size());
    }

private:
    std::vector<Object> objects_;
    std::vector<Id> freeIds_;
    NamedRegistry<Id, Key, Hash> names_;
};

}