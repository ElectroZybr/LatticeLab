#pragma once

#include <string>
#include <string_view>

#include <Lattice/Kernel/Value.hpp>

class Document {
public:
    Document() = default;

    explicit Document(Table root)
        : root_(std::move(root)) {}

    const Value* get(std::string_view key) const {
        auto it = root_.find(std::string(key));

        if (it == root_.end())
            return nullptr;

        return &it->second;
    }

    Value* get(std::string_view key) {
        auto it = root_.find(std::string(key));

        if (it == root_.end())
            return nullptr;

        return &it->second;
    }

    const Table& root() const {
        return root_;
    }

    Table& root() {
        return root_;
    }

private:
    Table root_;
};