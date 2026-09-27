#pragma once

#include <algorithm>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <Lattice/Kernel/TableAPI.hpp>
#include <Lattice/Kernel/NodeViews.hpp>


namespace Lattice {

/**
 * Default owning table implementation supplied by the kernel.
 *
 * Every column is a typed contiguous array. The implementation is intended
 * for general-purpose data; specialized plugins may provide another Table
 * implementation without changing consumers.
 */
 
class BasicTable final : public Table {
    class ColumnStorage {
    public:
        template<typename T>
        ColumnStorage(std::in_place_type_t<T>, std::string name, size_t size)
            : name_(std::move(name)),
              type_(tableType<T>()),
              data_(new T[size]),
              size_(size),
              destroy_([](void* data) noexcept {
                  delete[] static_cast<T*>(data);
              }),
              resize_([](void* data, size_t oldSize, size_t newSize) -> void* {
                  auto values = std::make_unique<T[]>(newSize);
                  auto* oldValues = static_cast<T*>(data);
                  const size_t preserved = std::min(oldSize, newSize);
                  for (size_t index = 0; index < preserved; ++index)
                      values[index] = std::move_if_noexcept(oldValues[index]);
                  delete[] oldValues;
                  return values.release();
              }) {}

        ColumnStorage(const ColumnStorage&) = delete;
        ColumnStorage& operator=(const ColumnStorage&) = delete;

        ColumnStorage(ColumnStorage&& other) noexcept
            : name_(std::move(other.name_)),
              type_(other.type_),
              data_(std::exchange(other.data_, nullptr)),
              size_(std::exchange(other.size_, 0)),
              destroy_(other.destroy_),
              resize_(other.resize_) {}

        ColumnStorage& operator=(ColumnStorage&&) = delete;

        ~ColumnStorage() {
            if (data_)
                destroy_(data_);
        }

        std::string_view name() const noexcept { return name_; }
        TableType type() const noexcept { return type_; }
        const void* data() const noexcept { return data_; }
        void* data() noexcept { return data_; }
        size_t size() const noexcept { return size_; }

        void resize(size_t size) {
            if (size == size_)
                return;
            data_ = resize_(data_, size_, size);
            size_ = size;
        }

    private:
        using Destroy = void (*)(void*) noexcept;
        using Resize = void* (*)(void*, size_t, size_t);

        std::string name_;
        TableType type_;
        void* data_ = nullptr;
        size_t size_ = 0;
        Destroy destroy_ = nullptr;
        Resize resize_ = nullptr;
    };

public:
    BasicTable() = default;
    explicit BasicTable(::NodeBuild branch)
        : Table(std::move(branch)) {}
    BasicTable(const BasicTable&) = delete;
    BasicTable& operator=(const BasicTable&) = delete;
    BasicTable(BasicTable&&) noexcept = default;
    BasicTable& operator=(BasicTable&&) noexcept = default;

    size_t rows() const noexcept override { return rows_; }
    size_t columns() const noexcept override { return columns_.size(); }

    using Table::column;

    ColumnView column(size_t index) const override {
        if (index >= columns_.size())
            throw Exception(
                "BasicTable",
                "Column {} is out of range [0, {})",
                index,
                columns_.size()
            );

        const auto& storage = columns_[index];
        return {
            storage.name(),
            storage.type(),
            storage.data(),
            storage.size(),
            storage.type().size
        };
    }

    void resize(size_t rows) override {
        for (auto& column : columns_)
            column.resize(rows);
        rows_ = rows;
    }

    template<typename T>
    void addColumn(std::string name) {
        using Value = std::remove_cv_t<T>;
        static_assert(std::is_default_constructible_v<Value>,
            "BasicTable columns must be default constructible");
        static_assert(std::is_assignable_v<Value&, Value>,
            "BasicTable columns must be assignable");

        if (name.empty())
            throw Exception("BasicTable", "Column name cannot be empty");
        if (columnIndices_.contains(name))
            throw Exception("BasicTable", "Column '{}' already exists", name);

        const size_t index = columns_.size();
        columns_.emplace_back(std::in_place_type<Value>, std::move(name), rows_);
        try {
            columnIndices_.emplace(columns_.back().name(), index);
        } catch (...) {
            columns_.pop_back();
            throw;
        }
    }

    template<typename T>
    std::span<T> values(std::string_view name) {
        using Value = std::remove_cv_t<T>;
        ColumnStorage& storage = requireColumn(name);
        requireType<Value>(storage);
        return {static_cast<Value*>(storage.data()), rows_};
    }

    template<typename T>
    std::span<const T> values(std::string_view name) const {
        return column(name).template values<T>();
    }

protected:
    void* mutableElement(size_t column, size_t row) override {
        if (column >= columns_.size())
            throw Exception("BasicTable", "Column {} is out of range", column);
        if (row >= rows_)
            throw Exception("BasicTable", "Row {} is out of range", row);

        ColumnStorage& storage = columns_[column];
        return static_cast<std::byte*>(storage.data()) + row * storage.type().size;
    }

private:
    ColumnStorage& requireColumn(std::string_view name) {
        const auto found = columnIndices_.find(name);
        if (found == columnIndices_.end())
            throw Exception("BasicTable", "Column '{}' not found", name);
        return columns_[found->second];
    }

    template<typename T>
    static void requireType(const ColumnStorage& storage) {
        if (!storage.type().template is<T>())
            throw Exception(
                "BasicTable",
                "Column '{}' contains '{}', requested '{}'",
                storage.name(),
                storage.type().name,
                typeKey<T>()
            );
    }

    size_t rows_ = 0;
    std::deque<ColumnStorage> columns_;
    std::unordered_map<std::string_view, size_t> columnIndices_;
};

}
