#include <chrono>
#include <memory>
#include <thread>

#include <Lattice/Tools/BmRunner/Metrics/MemoryTraffic.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {
namespace {

class FakeMemoryTrafficBackend final
    : public Benchmarks::MemoryTrafficBackend {
public:
    size_t begins = 0;
    size_t ends = 0;

    bool available() const noexcept override {
        return true;
    }

    void begin() override {
        ++begins;
    }

    uint64_t end() override {
        ++ends;
        return 128;
    }
};

class UnavailableMemoryTrafficBackend final
    : public Benchmarks::MemoryTrafficBackend {
public:
    bool available() const noexcept override {
        return false;
    }

    std::string_view unavailableReason() const noexcept override {
        return "test backend is unavailable";
    }

    void begin() override {}
    uint64_t end() override { return 0; }
};

}

TEST(MemoryTraffic_ReportsBytesAndBandwidth, Fixture) {
    auto backend = std::make_unique<FakeMemoryTrafficBackend>();
    auto* observed = backend.get();
    Benchmarks::MemoryTraffic traffic(std::move(backend));

    traffic.begin();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const Benchmarks::Metrics sample = traffic.end();

    REQUIRE(observed->begins == 1);
    REQUIRE(observed->ends == 1);
    REQUIRE(sample.values.size() == Benchmarks::MemoryTraffic::_count);
    REQUIRE(sample.values[Benchmarks::MemoryTraffic::bytes] == 128.0);
    REQUIRE(sample.values[Benchmarks::MemoryTraffic::bandwidth] > 0.0);
    REQUIRE(
        sample.schema[Benchmarks::MemoryTraffic::bandwidth].unit ==
        Benchmarks::Unit::BytesPerSecond
    );

    const Benchmarks::Metrics result = traffic.result();

    REQUIRE(result.values.size() == Benchmarks::MemoryTraffic::_count);
    REQUIRE(result.values[Benchmarks::MemoryTraffic::bytes] == 128.0);
    REQUIRE(result.values[Benchmarks::MemoryTraffic::bandwidth] > 0.0);
    REQUIRE(traffic.result().values.empty());
}

TEST(MemoryTraffic_ReportsUnavailableBackend, Fixture) {
    Benchmarks::MemoryTraffic traffic(
        std::make_unique<UnavailableMemoryTrafficBackend>()
    );

    REQUIRE(!traffic.available());
    REQUIRE(
        traffic.unavailableReason() ==
        "test backend is unavailable"
    );
}

}
