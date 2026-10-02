#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <vector>

#include <Lattice/Tools/BmRunner/Benchmarks.hpp>
#include <Lattice/Tools/BmRunner/Metrics.hpp>


struct MemoryLatencyFixture : Lattice::Fixture {
    static constexpr size_t Steps = 1 << 18;

private:
    struct Dataset {
        std::vector<uint32_t> next;
        std::vector<uint32_t> starts;
        std::atomic<size_t> cursor{0};

        explicit Dataset(size_t elements)
            : next(elements) {
            std::vector<uint32_t> order(elements);
            std::iota(order.begin(), order.end(), 0);

            std::mt19937 rng(12345);
            std::shuffle(order.begin(), order.end(), rng);

            starts.reserve((elements + Steps - 1) / Steps);
            for (size_t offset = 0; offset < elements; offset += Steps)
                starts.push_back(order[offset]);

            for (size_t i = 0; i + 1 < elements; ++i)
                next[order[i]] = order[i + 1];

            next[order.back()] = order.front();
        }
    };

    static std::shared_ptr<Dataset> dataset(size_t elements) {
        static std::mutex mutex;
        static std::shared_ptr<Dataset> cached;

        std::lock_guard lock(mutex);
        if (!cached || cached->next.size() != elements)
            cached = std::make_shared<Dataset>(elements);

        return cached;
    }

    std::shared_ptr<Dataset> dataset_;

public:
    const std::vector<uint32_t>& next;
    uint32_t start = 0;

    explicit MemoryLatencyFixture(size_t bytes)
        : dataset_(dataset(std::max<size_t>(bytes / sizeof(uint32_t), 2)))
        , next(dataset_->next) {}

    void prepare() override {
        const size_t segment = dataset_->cursor.fetch_add(
            1,
            std::memory_order_relaxed
        );
        start = dataset_->starts[segment % dataset_->starts.size()];
    }
};

BENCH_GROUP(MemoryLatency) {
    bench.config.sizes = {
        4ull << 10,
        8ull << 10,
        16ull << 10,
        32ull << 10,
        64ull << 10,
        128ull << 10,
        256ull << 10,
        512ull << 10,

        1ull << 20,
        2ull << 20,
        4ull << 20,
        8ull << 20,
        16ull << 20,
        32ull << 20,
        64ull << 20,
        128ull << 20,
        256ull << 20
    };

    bench.stages.clear();

    bench.stages.add<Lattice::Benchmarks::Warmup>()
        .samples(4)
        .time(std::chrono::milliseconds(20));

    bench.stages.add<Lattice::Benchmarks::Time>()
        .samples(10);

    bench.analysis.clear();
    bench.analysis.growth(
        Lattice::Benchmarks::N,
        Lattice::Benchmarks::Time::perOperation
    );
}

BENCH_GROUPED(
    MemoryLatency,
    PointerChase,
    MemoryLatencyFixture,
    "dependent random memory access"
) {
    bench.measure<MemoryLatencyFixture>([](MemoryLatencyFixture& fixture) {
        uint32_t index = fixture.start;

        for (size_t i = 0; i < MemoryLatencyFixture::Steps; ++i)
            index = fixture.next[index];

        return index;
    });
}
