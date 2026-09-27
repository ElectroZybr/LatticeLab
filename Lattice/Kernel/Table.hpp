#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>

#include <Lattice/Kernel/Consts.hpp>
#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/TypeName.hpp>

namespace Lattice {

struct TableType {
    std::string_view name;
    size_t size = 0;
    size_t alignment = 0;

    template<typename T>
    bool is() const noexcept;

    friend bool operator==(const TableType&, const TableType&) = default;
};

template<typename T>
constexpr TableType tableType() noexcept {
    using Value = std::remove_cv_t<T>;
    static_assert(std::is_object_v<Value>);
    return {
        .name = typeKey<Value>(),
        .size = sizeof(Value),
        .alignment = alignof(Value)
    };
}

template<typename T>
bool TableType::is() const noexcept {
    return *this == tableType<T>();
}

/**
 * A non-owning view of one contiguous table column.
 *
 * The view and all spans obtained from it are invalidated when the table is
 * structurally changed or resized.
 */
class ColumnView {
public:
    ColumnView() = default;

    ColumnView(
        std::string_view name,
        TableType type,
        const void* data,
        size_t size,
        size_t stride
    ) noexcept
        : name_(name), type_(type), data_(data), size_(size), stride_(stride) {}

    std::string_view name() const noexcept { return name_; }
    TableType type() const noexcept { return type_; }
    size_t size() const noexcept { return size_; }
    size_t stride() const noexcept { return stride_; }
    bool empty() const noexcept { return size_ == 0; }

    const void* element(size_t row) const {
        if (row >= size_)
            throw Exception("Table", "Row {} is out of range [0, {})", row, size_);

        return static_cast<const std::byte*>(data_) + row * stride_;
    }

    template<typename T>
    std::span<const T> values() const {
        using Value = std::remove_cv_t<T>;
        if (!type_.is<Value>())
            throw Exception(
                "Table",
                "Column '{}' contains '{}', requested '{}'",
                name_,
                type_.name,
                typeKey<Value>()
            );
        if (stride_ != sizeof(Value))
            throw Exception("Table", "Column '{}' is not contiguous", name_);

        return {static_cast<const Value*>(data_), size_};
    }

private:
    std::string_view name_;
    TableType type_;
    const void* data_ = nullptr;
    size_t size_ = 0;
    size_t stride_ = 0;
};

/**
 * Read-only runtime contract for tabular data.
 *
 * Implementations own storage and mutation APIs. Consumers depend only on the
 * schema and non-owning column views exposed here.
 */
class Table : public Component {
public:
    virtual ~Table() = default;

    virtual size_t rows() const noexcept = 0;
    virtual size_t columns() const noexcept = 0;
    virtual ColumnView column(size_t index) const = 0;

    std::optional<size_t> findColumn(std::string_view name) const {
        for (size_t index = 0; index < columns(); ++index) {
            if (column(index).name() == name)
                return index;
        }
        return std::nullopt;
    }

    ColumnView column(std::string_view name) const {
        const auto index = findColumn(name);
        if (!index)
            throw Exception("Table", "Column '{}' not found", name);
        return column(*index);
    }
};

}
