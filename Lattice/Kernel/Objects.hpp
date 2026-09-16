#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <Lattice/Kernel/Exception.hpp>


namespace Lattice {

using ObjectId = uint32_t;
class Objects;
class Node;

inline constexpr ObjectId InvalidObjectId = std::numeric_limits<ObjectId>::max();

struct Object {
    std::string name;
    ObjectId parent = InvalidObjectId;
    Node* node = nullptr;
    bool exists = false;
};

class Objects {
public:
    ObjectId create(std::string_view name, ObjectId parent, Node* node) {
        ObjectId id;

        if (!freeIds.empty()) {
            id = freeIds.back();
            freeIds.pop_back();

            Object& entry = objects[id];
            entry.name = name;
            entry.parent = parent;
            entry.node = node;
            entry.exists = true;
        } else {
            id = static_cast<ObjectId>(objects.size());
            objects.push_back(Object{
                .name = std::string(name),
                .parent = parent,
                .node = node,
                .exists = true
            });
        }

        if (!name.empty())
            lookup.insert_or_assign(ObjectKey{std::string(name), parent}, id);
        return id;
    }

    static constexpr bool valid(ObjectId id) noexcept {
        return id != InvalidObjectId;
    }

    void alias(ObjectId id, std::string_view name, ObjectId parent) {
        if (!get(id))
            return;

        ObjectKey key = {std::string(name), parent};
        lookup.insert_or_assign(key, id);
    }

    void destroy(ObjectId id) {
        if (id >= objects.size())
            return;

        Object& entry = objects[id];

        // Удаляем все имена/алиасы, указывающие на этот объект.
        for (auto it = lookup.begin(); it != lookup.end();) {
            if (it->second == id)
                it = lookup.erase(it);
            else
                ++it;
        }

        entry.exists = false;
        entry.node = nullptr;
        entry.name.clear();

        freeIds.push_back(id);
    }

    const Object* get(ObjectId id) const {
        if (!valid(id) || id >= objects.size())
            return nullptr;

        const Object& entry = objects[id];

        if (!entry.exists)
            return nullptr;

        return &entry;
    }

    const Object& require(ObjectId id) const {
        const Object* entry = get(id);

        if (!entry)
            throw Lattice::Exception("Objects", "Object with id {} not found", id);

        return *entry;
    }

    const Object& operator[](ObjectId id) const {
        return require(id);
    }

    ObjectId find(std::string_view name, ObjectId parent) const {
        ObjectKey key = {std::string(name), parent};
        auto it = lookup.find(key);
        if (it == lookup.end() || !valid(it->second))
            return InvalidObjectId;
        return  it->second;
    }

    bool has(std::string_view name, ObjectId parent) const {
        return valid(find(name, parent));
    }

private:
    struct ObjectKey {
        std::string name;
        ObjectId parent;

        bool operator==(const ObjectKey&) const = default;
    };

    struct ObjectKeyHash {
        size_t operator()(const ObjectKey& key) const noexcept {
            size_t h = std::hash<ObjectId>{}(key.parent);
            h ^= std::hash<std::string>{}(key.name)
                + 0x9e3779b9
                + (h << 6)
                + (h >> 2);
            return h;
        }
    };

    std::vector<Object> objects;
    std::vector<ObjectId> freeIds;

    std::unordered_map<ObjectKey, ObjectId, ObjectKeyHash> lookup;
};

}