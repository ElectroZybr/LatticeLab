// GPU compute against the CPU on the same work: run `xmake run Wgpu.bench [name]`.
// Iterations are split into phases (BmRunner Trace): recording, submit, waiting, copies
// with their bytes, and the GPU time of the pass from timestamp queries (Execute@dev).
// Implementations of one work side by side: cpu (one thread and all, the Tasks::Workers
// axis) and gpu, each checked against cpu before it is measured; ScaleAddOffload measures
// data coming from memory (cold cache / copies there and back). A group run prints the
// crossovers N*.
#include <array>
#include <cmath>
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
#include <Tasks/Workers.hpp>

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

using Lattice::Benchmarks::Phase;
using Lattice::Benchmarks::Run;
using Lattice::Benchmarks::Trace;

/// The input of the Compute works: an array of ones.
using Array = std::vector<float>;

/// x = 1.0001·x + 1 on the host and on the GPU agree to rounding.
bool near(const Array& expected, const Array& actual) {
    if (expected.size() != actual.size())
        return false;
    for (size_t i = 0; i < expected.size(); ++i)
        if (std::abs(expected[i] - actual[i]) > 1e-5f * std::abs(expected[i]))
            return false;
    return true;
}

struct Values : Lattice::Fixture {
    using Input = Array;
    explicit Values(const Array& input) : host(input) {}
    std::vector<float> host;
};

/// The same array, out of every cache before each iteration (Offload on the CPU).
struct ColdValues : Values {
    using Values::Values;
    void prepare() override { Lattice::Benchmarks::evictCaches(); }
};

/// The same array resident on the GPU, with its pipeline ready; one per point N.
struct GpuValues : Lattice::Fixture {
    using Input = Array;
    static constexpr bool Reuse = true;
    explicit GpuValues(size_t n) : GpuValues(Array(n, 1.0f)) {}
    explicit GpuValues(const Array& input)
        : count(input.size()),
          host(input),
          data(device(), {.size = input.size() * sizeof(float), .usage = Wgpu::BufferUsage::Storage |
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

    /// The array on the device, for the check against cpu.
    Array output() {
        Array values(count);
        device().readBuffer(data, 0, std::as_writable_bytes(std::span(values)));
        return values;
    }
};

/// The CPU kernel over partitions of the array on `workers` threads.
void scaleAddOnCpu(std::vector<float>& values, size_t workers) {
    Tasks::benchPool().forPartitions(values.size(), workers, [&](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i)
            values[i] = values[i] * 1.0001f + 1.0f;
    });
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

BENCH_WORK(Compute, ScaleAdd, Array, "x = 1.0001·x + 1: data in the cache / on the device") {
    work.input([](size_t n) { return Array(n, 1.0f); });
    work.check(near);
    work.reference("cpu");
}

BENCH_IMPLEMENTATION(Compute, ScaleAdd, "cpu", Values) {
    impl.axis<Tasks::Workers>();
    impl.output([](Values& values) { return values.host; });
    impl.measure([](Values& values, Run& run) {
        auto execute = run.trace().span(Phase::Execute);
        scaleAddOnCpu(values.host, run.value<Tasks::Workers>());
    });
}

BENCH_IMPLEMENTATION(Compute, ScaleAdd, "gpu", GpuValues) {
    impl.output([](GpuValues& values) { return values.output(); });
    impl.measure([](GpuValues& values, Run& run) { scaleAddOnGpu(values, run.trace()); });
}

BENCH_WORK(Compute, ScaleAddOffload, Array,
           "same, data from memory: CPU from a cold cache, GPU with the copies there and back") {
    work.input([](size_t n) { return Array(n, 1.0f); });
    work.check(near);
    work.reference("cpu");
}

BENCH_IMPLEMENTATION(Compute, ScaleAddOffload, "cpu", ColdValues) {
    impl.axis<Tasks::Workers>();
    impl.output([](ColdValues& values) { return values.host; });
    impl.measure([](ColdValues& values, Run& run) {
        auto execute = run.trace().span(Phase::Execute);
        scaleAddOnCpu(values.host, run.value<Tasks::Workers>());
    });
}

BENCH_IMPLEMENTATION(Compute, ScaleAddOffload, "gpu", GpuValues) {
    // The result comes back into the host array, as an offloading step would leave it.
    impl.output([](GpuValues& values) { return values.host; });
    impl.measure([](GpuValues& values, Run& run) {
        Trace& trace = run.trace();
        const auto bytes = std::as_bytes(std::span(values.host));
        {
            // writeBuffer only stages; flush() makes the copy to the device part of this phase.
            auto upload = trace.span(Phase::TransferIn);
            device().writeBuffer(values.data, 0, bytes);
            device().flush();
            upload.bytes(bytes.size());
        }
        scaleAddOnGpu(values, trace);
        {
            auto readback = trace.span(Phase::TransferOut);
            device().readBuffer(values.data, 0, std::as_writable_bytes(std::span(values.host)));
            readback.bytes(bytes.size());
        }
    });
}

BENCH_GROUP(Transfer) { bench.config.sizes = Sizes; }

BENCH_GROUPED(Transfer, Upload, GpuValues, "writeBuffer of the whole array, submitted and waited") {
    bench.measure<GpuValues>([](GpuValues& values) {
        device().writeBuffer(values.data, 0, std::as_bytes(std::span(values.host)));
        device().flush();
    });
}

BENCH_GROUPED(Transfer, Readback, GpuValues, "readBuffer of the whole array into the host array") {
    bench.measure<GpuValues>([](GpuValues& values) {
        device().readBuffer(values.data, 0, std::as_writable_bytes(std::span(values.host)));
    });
}

// [filter] [--filter <group|bench|mask>] [--tsv <file>] [--sweep <axis>] [--axis <axis>=<v1,v2>]
// [--list]; a group run also compares the implementations of each work.
int main(int argc, char** argv) {
    return Lattice::Benchmarks::main(argc, argv);
}
