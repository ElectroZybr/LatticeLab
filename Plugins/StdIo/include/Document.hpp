#pragma once

#include <string>
#include <string_view>

#include <Lattice/Kernel/Value.hpp>


class Document {
public:
    Document() = default;

    explicit Document(Lattice::Object root)
        : root_(std::move(root)) {}

    const Lattice::Value* get(std::string_view key) const {
        auto it = root_.find(std::string(key));

        if (it == root_.end())
            return nullptr;

        return &it->second;
    }

    Lattice::Value* get(std::string_view key) {
        auto it = root_.find(std::string(key));

        if (it == root_.end())
            return nullptr;

        return &it->second;
    }

    const Lattice::Value* section(std::string_view name) const {
        return get(name);
    }

    const Lattice::Object& root() const {
        return root_;
    }

    Lattice::Object& root() {
        return root_;
    }

private:
    Lattice::Object root_;
};