#include "GpuSoA.hpp"

#include <algorithm>

#include <Lattice/tools/Exception.hpp>

#include "CommandList.hpp"

namespace Wgpu {

using Error = Lattice::Exception<GpuSoA>;

namespace {

constexpr uint64_t alignUp(uint64_t value, uint64_t alignment) noexcept {
    return (value + alignment - 1) / alignment * alignment;
}

constexpr BufferUsage tableUsage =
    BufferUsage::Storage | BufferUsage::CopyDestination | BufferUsage::CopySource;

bool sameLayout(const std::vector<GpuSoA::Column>& a, const std::vector<GpuSoA::Column>& b) {
    return std::ranges::equal(a, b, [](const auto& x, const auto& y) {
        return x.name == y.name && x.type == y.type && x.offset == y.offset;
    });
}

}

GpuSoA::GpuSoA(Device& device, std::string label)
    : device_(device), label_(std::move(label)) {
    allocate(4);
}

GpuSoA::GpuSoA(Device& device, CpuSource source, std::string label)
    : GpuSoA(device, std::move(label)) {
    source_ = &source.table;
    owner_ = &source.owner;
    attached_ = true;
    requestCopy();
}

GpuSoA::~GpuSoA() = default;

void GpuSoA::requestCopy() {
    if (!attached_ || !wanted() || pending_.exchange(true))
        return;
    owner_->post([this] {
        upload(*source_);
        pending_ = false;
    });
}

size_t GpuSoA::rows() const noexcept {
    std::lock_guard lock(mutex_);
    return rows_;
}

size_t GpuSoA::columns() const noexcept {
    std::lock_guard lock(mutex_);
    return columns_.size();
}

Lattice::ColumnView GpuSoA::column(size_t index) const {
    std::lock_guard lock(mutex_);
    if (index >= columns_.size())
        throw Error("Column {} is out of range [0, {})", index, columns_.size());
    const auto& column = columns_[index];
    // No host pointer: the column lives on the device.
    return {column.name, column.type, nullptr, rows_, column.type.size};
}

void* GpuSoA::mutableElement(size_t, size_t) {
    throw Error("GpuSoA '{}' is not host-accessible; write it on the GPU or upload a CPU table", label_);
}

uint64_t GpuSoA::offset(std::string_view name) const {
    std::lock_guard lock(mutex_);
    for (const auto& column : columns_)
        if (column.name == name)
            return column.offset;
    std::string names;
    for (const auto& column : columns_)
        names += (names.empty() ? "" : ", ") + column.name;
    throw Error("GpuSoA '{}' has no column '{}'; columns: [{}]", label_, name, names);
}

uint64_t GpuSoA::generation() const {
    std::lock_guard lock(mutex_);
    return generation_;
}

bool GpuSoA::wanted() const noexcept {
    std::lock_guard lock(mutex_);
    return taken_;
}

void GpuSoA::allocate(uint64_t bytes) {
    buffer_ = std::make_unique<Buffer>(device_, BufferDesc{
        .size = std::max<uint64_t>(alignUp(bytes, 4), 4), .usage = tableUsage, .label = label_});
    ++generation_;
}

void GpuSoA::addColumn(std::string name, Lattice::TableType type) {
    std::lock_guard lock(mutex_);
    if (std::ranges::any_of(columns_, [&](const Column& column) { return column.name == name; }))
        throw Error("GpuSoA '{}' already has column '{}'", label_, name);
    auto columns = columns_;
    columns.push_back({std::move(name), type, 0});
    relayout(capacity_, std::move(columns));
}

void GpuSoA::resize(size_t rows) {
    std::lock_guard lock(mutex_);
    if (rows > capacity_) {
        size_t capacity = capacity_ ? capacity_ : rows;
        while (capacity < rows)
            capacity = capacity * 3 / 2 + 1;
        relayout(capacity, columns_);
    }
    rows_ = rows;
}

// GPU-owned layout: offsets and column sizes are multiples of 4 (buffer copy rules).
void GpuSoA::relayout(size_t capacity, std::vector<Column> columns) {
    capacity = alignUp(capacity, 4);
    uint64_t total = 0;
    for (auto& column : columns) {
        total = alignUp(total, std::max<uint64_t>(column.type.alignment, 4));
        column.offset = total;
        total += column.type.size * capacity;
    }

    auto previous = std::move(buffer_);
    allocate(total);
    if (rows_) {
        CommandList commands{device_};
        for (const auto& column : columns)
            for (const auto& old : columns_)
                if (old.name == column.name)
                    commands.copyBuffer(*previous, old.offset, *buffer_, column.offset,
                        alignUp(column.type.size * rows_, 4));
        commands.submit();
    }
    columns_ = std::move(columns);
    capacity_ = capacity;
}

void GpuSoA::upload(const StdData::SoA& source) {
    std::vector<Column> columns;
    columns.reserve(source.columns());
    for (size_t i = 0; i < source.columns(); ++i) {
        const auto view = source.column(i);
        columns.push_back({std::string(view.name()), view.type(), source.columnOffset(i)});
    }
    const auto block = source.bytes();
    if (block.size() % 4)
        throw Error("GpuSoA '{}': source block of {} bytes is not whole 32-bit words", label_, block.size());

    std::lock_guard lock(mutex_);
    if (block.size() > buffer_->size() || !sameLayout(columns, columns_) || capacity_ != source.capacity()) {
        allocate(block.size() + block.size() / 2);
        columns_ = std::move(columns);
        capacity_ = source.capacity();
    }
    rows_ = source.rows();
    taken_ = false;

    // One writeBuffer: a frame sees either the previous upload or this one.
    if (!block.empty())
        device_.writeBuffer(*buffer_, 0, block);
}

}
