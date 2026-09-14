#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <limits>

#include <Lattice/Kernel/Exception.hpp>

namespace Lattice {

template<typename Object, typename Id, typename Key, typename Hash = std::hash<Key>>
class Registry {
public:
    Id create(Object object) {
        const Key key{object.name};

        if (has(key))
            throw Lattice::Exception("Registry", "Object '{}' already exists", object.name);

        const Id id = allocate();

        if (id == static_cast<Id>(objects_.size()))
            objects_.push_back(std::move(object));
        else
            objects_[id] = std::move(object);

        lookup_.emplace(key, id);
        return id;
    }

    static constexpr Id InvalidId = std::numeric_limits<Id>::max();

    static constexpr bool valid(Id id) noexcept {
        return id != InvalidId;
    }

    // добавляет псевдоним к слоту
    void alias(Id id, std::string_view name) {
        if (!get(id))
            return;

        const Key key{std::string(name)};

        if (lookup_.contains(key))
            throw Lattice::Exception("Registry", "Name '{}' already exists", name);

        lookup_.emplace(key, id);
    }

    // уничтожает слот
    void destroy(Id id) {
        if (!get(id))
            return;

        for (auto it = lookup_.begin(); it != lookup_.end();) {
            if (it->second == id)
                it = lookup_.erase(it);
            else
                ++it;
        }

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
        const auto it = lookup_.find(Key{std::string(name)});

        if (it == lookup_.end())
            return InvalidId;

        return it->second;
    }

    // проверяет наличие имени в реестре
    bool has(std::string_view name) const {
        return lookup_.contains(Key{std::string(name)});
    }

    void clear() {
        objects_.clear();
        freeIds_.clear();
        lookup_.clear();
    }

    Id size() const {
        return static_cast<Id>(objects_.size());
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
    std::unordered_map<Key, Id, Hash> lookup_;
};

}