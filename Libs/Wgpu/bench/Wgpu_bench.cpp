// GPU compute against the CPU on the same work: run `xmake run Wgpu.bench [name]`.
// GPU timings include recording, submit and waiting for completion.
#include <array>
#include <memory>
#include <span>
#include <vector>

#include <BmRunner/Benchmarks.hpp>
#include <Wgpu/CommandList.hpp>
#include <Wgpu/ComputePass.hpp>
#include <Wgpu/Device.hpp>
#include <Wgpu/Resources.hpp>

namespace {

const std::vector<size_t> Sizes = {1'000, 10'000, 100'000, 1'000'000, 10'000'000};

const char* doubleShader = R"(
@group(0) @binding(0) var<storage, read_write> data : array<f32>;
@compute @workgroup_size(256)
fn main(@builtin(global_invocation_id) id : vec3<u32>) {
    if (id.x < arrayLength(&data)) {
        data[id.x] = data[id.x] * 1.0001 + 1.0;
    }
}
)";

/// One device for the whole run (creating it costs far more than any measured step).
Wgpu::Device& device() {
    static Wgpu::Instance instance;
    static Wgpu::Device device{instance};
    return device;
}

struct Values : Lattice::Fixture {
    explicit Values(size_t n) : host(n, 1.0f) {}
    std::vector<float> host;
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
          shader(device(), {.source = doubleShader, .label = "bench/double"}),
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
};

}

BENCH_GROUP(Compute) { bench.config.sizes = Sizes; }

BENCH_GROUPED(Compute, ScaleAddCpu, Values, "x = 1.0001·x + 1 on the CPU") {
    bench.measure<Values>([](Values& values) {
        for (float& x : values.host)
            x = x * 1.0001f + 1.0f;
    });
}

BENCH_GROUPED(Compute, ScaleAddGpu, GpuValues, "same on the GPU: record, submit, wait") {
    bench.measure<GpuValues>([](GpuValues& values) {
        Wgpu::CommandList commands{device()};
        {
            Wgpu::ComputePass pass{commands};
            pass.setPipeline(values.pipeline);
            pass.setBindings(0, values.set);
            pass.dispatch(Wgpu::workgroups(values.count, 256));
        }
        commands.submit();
        device().wait();
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

int main(int argc, char** argv) {
    if (argc > 1)
        Lattice::Benchmarks::run(argv[1]);
    else
        Lattice::Benchmarks::runAll();
}
