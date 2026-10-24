// GPU compute against the CPU on the same work: run `xmake run Wgpu.bench [name]`.
// Iterations are split into phases (BmRunner Trace): recording, submit, waiting, copies
// with their bytes, and the GPU time of the pass from timestamp queries (Execute@dev).
// Pairs of one work: Resident (data stays on the device / in the cache) and Offload
// (copied there and back / cold cache); the group run prints the crossover N*.
#include <array>
#include <memory>
#include <span>
#include <vector>

#include <BmRunner/Benchmarks.hpp>
#include <BmRunner/Main.hpp>
#include <BmRunner/Evict.hpp>
#include <Wgpu/CommandList.hpp>
#include <Wgpu/ComputePass.hpp>
#include <Wgpu/Device.hpp>
#include <Wgpu/Resources.hpp>
#include <Wgpu/Timer.hpp>

namespace {

const std::vector<size_t> Sizes = {1'000, 10'000, 100'000, 1'000'000, 10'000'000};

constexpr unsigned char DoubleShader[] = {
#include "double.wgsl.h"
};

/// One device for the whole run (creating it costs far more than any measured step).
Wgpu::Device& device() {
    static Wgpu::Instance instance;
    static Wgpu::Device device{instance};
    return device;
}

using Lattice::Benchmarks::Mode;
using Lattice::Benchmarks::Phase;
using Lattice::Benchmarks::Trace;

struct Values : Lattice::Fixture {
    explicit Values(size_t n) : host(n, 1.0f) {}
    std::vector<float> host;
};

/// The same array, out of every cache before each iteration (Offload on the CPU).
struct ColdValues : Values {
    using Values::Values;
    void prepare() override { Lattice::Benchmarks::evictCaches(); }
};

/// The same array resident on the GPU, with its pipeline ready.
struct GpuValues : Lattice::Fixture {
    explicit GpuValues(size_t n)
        : count(n),
          host(n, 1.0f),
          data(device(), {.size = n * sizeof(float), .usage = Wgpu::BufferUsage::Storage |
              Wgpu::BufferUsage::CopyDestination | Wgpu::BufferUsage::CopySource, .label = "bench/data"}),
          layout(device(), std::array<Wgpu::LayoutEntry, 1>{{{.binding = 0, .type = Wgpu::BindingType::Storage,
              .visibility = Wgpu::ShaderStage::Compute}}}, "bench/data"),
          set(device(), layout, std::array{Wgpu::Binding{.binding = 0, .buffer = &data}}),
          shader(device(), {.source = reinterpret_cast<const char*>(DoubleShader), .label = "bench/double"}),
          pipeline(device(), shader, {.layouts = {&layout}, .label = "bench/double"}) {
        device().writeBuffer(data, 0, std::as_bytes(std::span(host)));
        device().wait();
    }

    size_t count;
    std::vector<float> host;
    Wgpu::Buffer data;
    Wgpu::BindingLayout layout;
    Wgpu::BindingSet set;
    Wgpu::Shader shader;
    Wgpu::ComputePipeline pipeline;
    Wgpu::Timer timer{device(), 1024, "bench/timer"};
};

void scaleAdd(std::vector<float>& values) {
    for (float& x : values)
        x = x * 1.0001f + 1.0f;
}

/// Record, submit and wait for one pass; the GPU time comes from the timer.
void scaleAddOnGpu(GpuValues& values, Trace& trace) {
    Wgpu::CommandList commands{device()};
    {
        auto prepare = trace.span(Phase::Prepare);
        Wgpu::ComputePass pass{commands, "scaleAdd", &values.timer};
        pass.setPipeline(values.pipeline);
        pass.setBindings(0, values.set);
        pass.dispatch(Wgpu::workgroups(values.count, 256));
    }
    {
        auto issue = trace.span(Phase::Issue);
        commands.submit();
    }
    {
        auto complete = trace.span(Phase::Complete);
        device().wait();
    }
    trace.deferIntervals(values.timer);
}

}

BENCH_GROUP(Compute) { bench.config.sizes = Sizes; }

BENCH_GROUPED(Compute, ScaleAddCpu, Values, "x = 1.0001·x + 1 on the CPU, data in the cache") {
    bench.work("ScaleAdd").executor("cpu").mode(Mode::Resident);
    bench.measure<Values>([](Values& values, Trace& trace) {
        auto execute = trace.span(Phase::Execute);
        scaleAdd(values.host);
    });
}

BENCH_GROUPED(Compute, ScaleAddGpu, GpuValues, "same on the GPU, data on the device") {
    bench.work("ScaleAdd").executor("gpu").mode(Mode::Resident);
    bench.measure<GpuValues>(scaleAddOnGpu);
}

BENCH_GROUPED(Compute, ScaleAddCpuCold, ColdValues, "on the CPU from a cold cache") {
    bench.work("ScaleAdd").executor("cpu").mode(Mode::Offload);
    bench.measure<ColdValues>([](ColdValues& values, Trace& trace) {
        auto execute = trace.span(Phase::Execute);
        scaleAdd(values.host);
    });
}

BENCH_GROUPED(Compute, ScaleAddGpuOffload, GpuValues, "on the GPU with the copies there and back") {
    bench.work("ScaleAdd").executor("gpu").mode(Mode::Offload);
    bench.measure<GpuValues>([](GpuValues& values, Trace& trace) {
        const auto bytes = std::as_bytes(std::span(values.host));
        {
            auto upload = trace.span(Phase::TransferIn);
            device().writeBuffer(values.data, 0, bytes);
            upload.bytes(bytes.size());
        }
        scaleAddOnGpu(values, trace);
        {
            auto readback = trace.span(Phase::TransferOut);
            const auto result = device().readBuffer(values.data, 0, bytes.size());
            readback.bytes(result.size());
        }
    });
}

BENCH_GROUP(Transfer) { bench.config.sizes = Sizes; }

BENCH_GROUPED(Transfer, Upload, GpuValues, "writeBuffer of the whole array, waited") {
    bench.measure<GpuValues>([](GpuValues& values) {
        device().writeBuffer(values.data, 0, std::as_bytes(std::span(values.host)));
        device().wait();
    });
}

BENCH_GROUPED(Transfer, Readback, GpuValues, "readBuffer of the whole array") {
    bench.measure<GpuValues>([](GpuValues& values) {
        return device().readBuffer(values.data, 0, values.count * sizeof(float)).size();
    });
}

// [filter] [--filter <group|bench|mask>] [--tsv <file>] [--list]; a group run also
// compares its executors.
int main(int argc, char** argv) {
    return Lattice::Benchmarks::main(argc, argv);
}
