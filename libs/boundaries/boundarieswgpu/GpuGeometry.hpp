#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include <boundaries/Program.hpp>
#include <Lattice/tools/Exception.hpp>
#include <wgpu/Device.hpp>
#include <wgpu/Resources.hpp>

namespace Boundaries::Wgpu {

/// Stack depth of the GPU machine (GEOMETRY_DEPTH in Geometry.wgsl): deeper programs are rejected.
constexpr uint32_t MaxDepth = 16;

/// A geometry program on the device: the instruction buffer the WGSL machine reads.
/// upload() replaces it (a new shape, animated parameters); the generation grows when
/// the buffer is recreated, so bind groups know to follow.
class GpuGeometry final {
public:
    GpuGeometry(::Wgpu::Device& device, Program program) : device_(device) { upload(std::move(program)); }

    void upload(Program program) {
        using Error = Lattice::Exception<GpuGeometry>;
        if (program.code.empty())
            throw Error("geometry program is empty");
        if (program.depth > MaxDepth)
            throw Error("geometry needs a stack of {}, the GPU machine has {}", program.depth, MaxDepth);
        const uint64_t bytes = program.code.size() * sizeof(Instruction);
        if (!code_ || code_->size() != bytes) {
            // The buffer is exactly the program: the machine runs arrayLength() instructions.
            code_ = std::make_unique<::Wgpu::Buffer>(device_, ::Wgpu::BufferDesc{.size = bytes,
                .usage = ::Wgpu::BufferUsage::Storage | ::Wgpu::BufferUsage::CopyDestination, .label = "geometry/code"});
            ++generation_;
        }
        device_.writeBuffer(*code_, 0, std::as_bytes(std::span(program.code)));
        program_ = std::move(program);
        ++version_;
    }

    const Program& program() const noexcept { return program_; }
    ::Wgpu::Buffer& code() const noexcept { return *code_; }
    uint64_t generation() const noexcept { return generation_; }
    /// Grows with every upload: the program's contents changed.
    uint64_t version() const noexcept { return version_; }
    ::Wgpu::Device& device() const noexcept { return device_; }

private:
    ::Wgpu::Device& device_;
    Program program_;
    std::unique_ptr<::Wgpu::Buffer> code_;
    uint64_t generation_ = 0;
    uint64_t version_ = 0;
};

}
