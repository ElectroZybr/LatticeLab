#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

class DynamicSoA {
    struct Column {
        size_t offset = 0;
        size_t elementSize = 0;
        size_t alignment = 0;
        const void* typeKey = nullptr;
        bool active = false;
    };

public:
    DynamicSoA() = default;
    DynamicSoA(const DynamicSoA&) = delete;
    DynamicSoA& operator=(const DynamicSoA&) = delete;

    DynamicSoA(DynamicSoA&& other) noexcept
        : size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)),
          storageBytes_(std::exchange(other.storageBytes_, 0)),
          storage_(std::exchange(other.storage_, nullptr)),
          columns_(std::move(other.columns_)) {}

    DynamicSoA& operator=(DynamicSoA&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        releaseStorage();
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
        storageBytes_ = std::exchange(other.storageBytes_, 0);
        storage_ = std::exchange(other.storage_, nullptr);
        columns_ = std::move(other.columns_);
        return *this;
    }

    ~DynamicSoA() { releaseStorage(); }

    template<typename T>
    void add(uint32_t id) {
        static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);
        if (id >= columns_.size()) {
            columns_.resize(static_cast<size_t>(id) + 1);
        }

        Column& col = columns_[id];
        col.elementSize = sizeof(T);
        col.alignment = alignof(T);
        col.typeKey = typeToken<T>();
        col.active = true;
        relayout(capacity_);
    }

    void clear() noexcept { size_ = 0; }

    void reserve(size_t required) {
        if (required <= capacity_) {
            return;
        }

        size_t newCapacity = capacity_ == 0 ? required : capacity_;
        while (newCapacity < required) {
            newCapacity = newCapacity * 3 / 2 + 1;
        }
        relayout(newCapacity);
    }

    void resize(size_t n) {
        reserve(n);
        size_ = n;
    }

    [[nodiscard]] size_t size() const noexcept { return size_; }
    [[nodiscard]] size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] size_t storageBytes() const noexcept { return storageBytes_; }

    template<typename T>
    [[nodiscard]] T* column(uint32_t id) noexcept {
        return reinterpret_cast<T*>(storage_ + require<T>(id).offset);
    }

    template<typename T>
    [[nodiscard]] const T* column(uint32_t id) const noexcept {
        return reinterpret_cast<const T*>(storage_ + require<T>(id).offset);
    }

    template<typename T>
    [[nodiscard]] std::span<T> span(uint32_t id) noexcept {
        return {column<T>(id), size_};
    }

    template<typename T>
    [[nodiscard]] std::span<const T> span(uint32_t id) const noexcept {
        return {column<T>(id), size_};
    }

    void swapRows(size_t a, size_t b) noexcept {
        if (a == b) {
            return;
        }

        std::byte scratch[32];
        for (const Column& col : columns_) {
            if (!col.active) {
                continue;
            }

            std::byte* lhs = storage_ + col.offset + a * col.elementSize;
            std::byte* rhs = storage_ + col.offset + b * col.elementSize;

            if (col.elementSize <= sizeof(scratch)) {
                std::memcpy(scratch, lhs, col.elementSize);
                std::memcpy(lhs, rhs, col.elementSize);
                std::memcpy(rhs, scratch, col.elementSize);
            } else {
                std::vector<std::byte> tmp(col.elementSize);
                std::memcpy(tmp.data(), lhs, col.elementSize);
                std::memcpy(lhs, rhs, col.elementSize);
                std::memcpy(rhs, tmp.data(), col.elementSize);
            }
        }
    }

private:
    template<typename T>
    static const void* typeToken() noexcept {
        static int token;
        return &token;
    }

    static size_t alignUp(size_t value, size_t alignment) noexcept {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    template<typename T>
    const Column& require(uint32_t id) const noexcept {
        assert(id < columns_.size());
        const Column& col = columns_[id];
        assert(col.active);
        assert(col.typeKey == typeToken<T>());
        return col;
    }

    static std::byte* allocateAligned(size_t bytes) {
        if (bytes == 0) {
            return nullptr;
        }
        return static_cast<std::byte*>(::operator new(bytes, std::align_val_t{alignof(std::max_align_t)}));
    }

    static void deallocateAligned(std::byte* storage, size_t bytes) noexcept {
        if (storage == nullptr) {
            return;
        }
        ::operator delete(storage, bytes, std::align_val_t{alignof(std::max_align_t)});
    }

    void releaseStorage() noexcept {
        deallocateAligned(storage_, storageBytes_);
        storage_ = nullptr;
        storageBytes_ = 0;
        capacity_ = 0;
    }

    void relayout(size_t newCapacity) {
        const std::vector<Column> oldColumns = columns_;
        std::byte* oldStorage = storage_;
        const size_t oldStorageBytes = storageBytes_;

        size_t totalBytes = 0;
        for (Column& col : columns_) {
            if (!col.active) {
                continue;
            }
            totalBytes = alignUp(totalBytes, col.alignment);
            col.offset = totalBytes;
            totalBytes += col.elementSize * newCapacity;
        }

        std::byte* newStorage = allocateAligned(totalBytes);
        if (newStorage != nullptr) {
            std::memset(newStorage, 0, totalBytes);
        }

        if (oldStorage != nullptr && size_ > 0) {
            for (size_t i = 0; i < columns_.size(); ++i) {
                const Column& oldCol = oldColumns[i];
                const Column& newCol = columns_[i];
                if (!oldCol.active) {
                    continue;
                }
                std::memcpy(newStorage + newCol.offset, oldStorage + oldCol.offset, oldCol.elementSize * size_);
            }
        }

        deallocateAligned(oldStorage, oldStorageBytes);
        storage_ = newStorage;
        storageBytes_ = totalBytes;
        capacity_ = newCapacity;
    }

    size_t size_ = 0;
    size_t capacity_ = 0;
    size_t storageBytes_ = 0;
    std::byte* storage_ = nullptr;
    std::vector<Column> columns_;
};
