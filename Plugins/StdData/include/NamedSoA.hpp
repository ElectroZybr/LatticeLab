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

class NamedSoA {
public:
    NamedSoA() = default;
    NamedSoA(const NamedSoA&) = delete;
    NamedSoA& operator=(const NamedSoA&) = delete;
    NamedSoA(NamedSoA&&) noexcept = default;
    NamedSoA& operator=(NamedSoA&&) noexcept = default;
    ~NamedSoA() = default;

    SoA& soa() noexcept { return soa_; }
    const SoA& soa() const noexcept { return soa_; }

    [[nodiscard]] size_t size() const noexcept { return soa_.size(); }
    [[nodiscard]] size_t capacity() const noexcept { return soa_.capacity(); }
    [[nodiscard]] size_t storageBytes() const noexcept { return soa_.storageBytes(); }

    void clear() noexcept {
        soa_.clear();
        names_.clear();
    }

    void reserve(size_t required) {
        soa_.reserve(required);
    }

    template<class Tag>
    typename Tag::type* addCol() {
        return soa_.addCol<Tag>();
    }

    template<class Tag>
    void remove() {
        soa_.remove<Tag>();
    }

    template<class Tag>
    [[nodiscard]] typename Tag::type* get() noexcept {
        return soa_.get<Tag>();
    }

    template<class Tag>
    [[nodiscard]] const typename Tag::type* get() const noexcept {
        return soa_.get<Tag>();
    }

    [[nodiscard]] void* get(std::string_view name) noexcept {
        return soa_.get(name);
    }

    [[nodiscard]] const void* get(std::string_view name) const noexcept {
        return soa_.get(name);
    }

    [[nodiscard]] bool has(std::string_view name) const noexcept {
        return soa_.has(name);
    }

    template<class Tag>
    [[nodiscard]] typename Tag::type* require() {
        return soa_.require<Tag>();
    }

    template<class Tag>
    [[nodiscard]] const typename Tag::type* require() const {
        return soa_.require<Tag>();
    }

    template<class Tag>
    [[nodiscard]] std::span<typename Tag::type> span() noexcept {
        return soa_.span<Tag>();
    }

    template<class Tag>
    [[nodiscard]] std::span<const typename Tag::type> span() const noexcept {
        return soa_.span<Tag>();
    }

    template<class Tag>
    [[nodiscard]] typename Tag::type& at(size_t index) noexcept {
        return soa_.at<Tag>(index);
    }

    template<class Tag>
    [[nodiscard]] const typename Tag::type& at(size_t index) const noexcept {
        return soa_.at<Tag>(index);
    }

    void set(std::string_view column, size_t index, const Lattice::Value& value) {
        soa_.set(column, index, value);
    }

    [[nodiscard]] size_t find(std::string_view name) const noexcept {
        return names_.find(name);
    }

    [[nodiscard]] bool hasRow(std::string_view name) const noexcept {
        return names_.has(name);
    }

    [[nodiscard]] size_t addRow(std::string_view name) {
        const size_t id = soa_.size();
        names_.add(name, id);
        soa_.resize(id + 1);
        return id;
    }

    [[nodiscard]] size_t requireRow(std::string_view name) const {
        const size_t id = find(name);

        if (!Lattice::NamedRegistry<size_t>::valid(id))
            throw Lattice::Exception("NamedSoA", "Row '{}' not found", name);

        return id;
    }

    void inspect(std::string_view label = "NamedSoA") const {
        soa_.inspect(label);
    }

private:
    std::string_view nameAt(size_t row) const {
        const auto* data = static_cast<const std::array<char, 64>*>(soa_.get("Name"));

        if (!data)
            throw Lattice::Exception("NamedSoA", "Name column not found");

        const auto& value = data[row];
        return std::string_view(value.data(), strnlen(value.data(), value.size()));
    }

    SoA soa_;
    Lattice::NamedRegistry<size_t> names_;
};

}