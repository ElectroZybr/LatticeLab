#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include <Lattice/Kernel/TableAPI.hpp>
#include <StdData/SoA.hpp>

#include "Device.hpp"
#include "Resources.hpp"

namespace WGPU {

/// SoA table on the GPU: one storage buffer, every column at its byte offset with
/// capacity() rows. The schema is the common Lattice::Table; column data is not
/// host-accessible (ColumnView without data).
///
/// Two ways to fill it:
/// * upload(soa) — mirror of a CPU StdData::SoA: same layout, the whole block in one
///   writeBuffer, so a frame never sees a half-updated table;
/// * addColumn + resize — a table owned on the GPU (a GPU simulation writes it itself).
///
/// The writer may run on another thread than the reader (render).
class GpuSoA final : public Lattice::Table {
public:
    struct Column {
        std::string name;
        Lattice::TableType type;
        uint64_t offset = 0;
    };

    /// What the reader binds: the buffer is valid while `generation` is unchanged.
    struct View {
        Buffer& buffer;
        uint64_t generation;
        size_t rows;
        size_t capacity;
        const std::vector<Column>& columns;
    };

    GpuSoA(Device& device, std::string label);
    ~GpuSoA() override;

    // Lattice::Table
    size_t rows() const noexcept override;
    size_t columns() const noexcept override;
    Lattice::ColumnView column(size_t index) const override;
    /// GPU-owned table: grow capacity, keeping the rows already on the device.
    void resize(size_t rows) override;
    using Lattice::Table::column;

    /// GPU-owned table: add a column; the layout is rebuilt (rows are kept).
    void addColumn(std::string name, Lattice::TableType type);

    /// Mirror `source`: schema, layout and rows follow it. The buffer is recreated
    /// only when the block outgrows it or the layout changes (generation grows).
    void upload(const StdData::SoA& source);

    /// Writer: true when the reader has taken the last upload — upload no more often
    /// than frames are drawn.
    bool wanted() const noexcept;

    /// Reader: call use(const View&) under the table lock and mark the upload taken.
    template<class Use>
    void read(Use&& use) {
        std::lock_guard lock(mutex_);
        use(View{*buffer_, generation_, rows_, capacity_, columns_});
        taken_ = true;
    }

    /// Byte offset of a column in the buffer.
    uint64_t offset(std::string_view column) const;
    uint64_t generation() const;

protected:
    /// Throws: GPU columns are not writable from the host.
    void* mutableElement(size_t column, size_t row) override;

private:
    void relayout(size_t capacity, std::vector<Column> columns);
    void allocate(uint64_t bytes);

    Device& device_;
    std::string label_;
    mutable std::mutex mutex_;
    std::vector<Column> columns_;
    std::unique_ptr<Buffer> buffer_;
    size_t rows_ = 0;
    size_t capacity_ = 0;
    uint64_t generation_ = 0;
    bool taken_ = true;
};

}
