#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <Lattice/Kernel/Exception.hpp>


namespace Lattice {

using ObjectId = uint32_t;
class ObjectRegistry;

inline constexpr ObjectId InvalidObjectId =
    std::numeric_limits<ObjectId>::max();

constexpr bool valid(ObjectId id) noexcept {
    return id != InvalidObjectId;
}

struct Entry {
    void* object = nullptr;
    std::string type;
    std::string name;
    ObjectId parent = InvalidObjectId;
};

class Path {
public:
    Path() = default;

    explicit Path(ObjectId id, ObjectRegistry& objectRegistry);

    std::span<const ObjectId> ids() const {
        return ids_;
    }

    bool empty() const {
        return ids_.empty();
    }

    size_t size() const {
        return ids_.size();
    }

    ObjectId operator[](size_t index) const {
        return ids_[index];
    }

    void push(ObjectId id) {
        ids_.push_back(id);
    }

    void pop() {
        ids_.pop_back();
    }

private:
    std::vector<ObjectId> ids_;
};


class ObjectRegistry {
public:
    ObjectId create(ObjectId parent, std::string_view type, std::string_view name, void* object) {
        ObjectId id;

        if (!freeIds.empty()) {
            id = freeIds.back();
            freeIds.pop_back();

            Entry& entry = objects[id];

            entry.object = object;
            entry.type = type;
            entry.name = name;
            entry.parent = parent;
        }
        else {
            id = static_cast<ObjectId>(objects.size());

            objects.push_back(Entry{
                .object = object,
                .type = std::string(type),
                .name = std::string(name),
                .parent = parent,
            });
        }

        ObjectKey key = {parent, std::string(type), std::string(name)};
        lookup.insert_or_assign(key, id);

        return id;
    }

    void alias(ObjectId id, ObjectId parent, std::string_view type, std::string_view name) {
        if (!get(id))
            return;

        ObjectKey key = {parent, std::string(type), std::string(name)};
        lookup.insert_or_assign(key, id);
    }

    void destroy(ObjectId id) {
        if (id >= objects.size())
            return;

        Entry& entry = objects[id];

        if (!entry.object)
            return;

        // Удаляем все имена/алиасы, указывающие на этот объект.
        for (auto it = lookup.begin(); it != lookup.end();) {
            if (it->second == id)
                it = lookup.erase(it);
            else
                ++it;
        }

        entry.object = nullptr;
        entry.type.clear();
        entry.name.clear();

        freeIds.push_back(id);
    }

    const Entry* get(ObjectId id) const {
        if (!valid(id) || id >= objects.size())
            return nullptr;

        const Entry& entry = objects[id];

        if (!entry.object)
            return nullptr;

        return &entry;
    }

    const Entry& require(ObjectId id) const {
        const Entry* entry = get(id);

        if (!entry)
            throw Lattice::Exception("ObjectRegistry", "Object with id {} not found", id);

        return *entry;
    }

    const Entry& operator[](ObjectId id) const {
        return require(id);
    }

    ObjectId find(ObjectId parent, std::string_view type, std::string_view name) const {
        ObjectKey key = {parent, std::string(type), std::string(name)};
        auto it = lookup.find(key);
        if (it == lookup.end() || !valid(it->second))
            return InvalidObjectId;
        return  it->second;
    }

    std::vector<ObjectId> path(ObjectId id) const {
        std::vector<ObjectId> result;

        while (valid(id)) {
            result.push_back(id);
            id = require(id).parent;
        }

        std::ranges::reverse(result);
        return result;
    }

    const std::string stringPath(ObjectId id) const {
        std::string result;

        for (ObjectId current : path(id)) {
            const auto& entry = require(current);

            if (!result.empty())
                result += '/';

            if (entry.name.empty() || entry.name == "default" || entry.name == "Root")
                result += std::format("{}", entry.type);
            else
                result += std::format("{}:{}", entry.type, entry.name);
        }

        return result;
    }

private:
    struct ObjectKey {
        ObjectId parent;
        std::string type;
        std::string name;

        bool operator==(const ObjectKey&) const = default;
    };

    struct ObjectKeyHash {
        size_t operator()(const ObjectKey& key) const noexcept {
            size_t h = std::hash<ObjectId>{}(key.parent);
            h ^= std::hash<std::string>{}(key.type)
                + 0x9e3779b9
                + (h << 6)
                + (h >> 2);
            h ^= std::hash<std::string>{}(key.name)
                + 0x9e3779b9
                + (h << 6)
                + (h >> 2);
            return h;
        }
    };

    std::vector<Entry> objects;
    std::vector<ObjectId> freeIds;

    std::unordered_map<ObjectKey, ObjectId, ObjectKeyHash> lookup;
};

}