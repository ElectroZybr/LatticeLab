#include <algorithm>
#include <unordered_map>
#include <vector>

#include <Lattice/Tools/BmRunner/Benchmarks.hpp>
#include <Lattice/Tools/BmRunner/Metrics.hpp>

using namespace Lattice;
using namespace Lattice::Benchmarks;
using namespace std;

class Solution {
public:
    vector<int> twoSumHash(vector<int>& nums, int target) {
        unordered_map<int, int> seen;

        for (int i = 0; i < nums.size(); ++i) {
            const int need = target - nums[i];

            if (seen.contains(need))
                return {seen[need], i};

            seen[nums[i]] = i;
        }

        return {};
    }

    vector<int> twoSumBrut(vector<int>& nums, int target) {
        for (int i = 0; i < nums.size(); ++i)
            for (int j = i + 1; j < nums.size(); ++j)
                if (nums[i] + nums[j] == target)
                    return {i, j};

        return {};
    }
};

struct TwoSumFixture : Fixture {
    vector<int> nums;

    explicit TwoSumFixture(size_t n)
        : nums(n) {}

    void prepare() override {
        std::fill(nums.begin(), nums.end(), 1);
        nums[nums.size() - 2] = 123;
        nums[nums.size() - 1] = 456;
    }
};

BENCH_GROUP(TwoSum) {
    bench.config.sizes = {10, 50, 100, 500, 1000, 5000, 10000};

    bench.stages.clear();
    bench.stages.add<Warmup>()
        .samples(16)
        .time(std::chrono::milliseconds(10));

    bench.stages.add<Time, Perf>()
        .samples(10);
    bench.stages.add<MemoryTraffic>()
        .samples(10);

    bench.analysis.growth(N, MemoryTraffic::bytes);
}

BENCH_GROUPED(TwoSum, Hash, TwoSumFixture, "unordered_map implementation") {
    bench.measure<TwoSumFixture>([](TwoSumFixture& fixture) {
        Solution solution;
        return solution.twoSumHash(fixture.nums, 579);
    });
}

BENCH_GROUPED(TwoSum, Brut, TwoSumFixture, "bruteforce implementation") {
    bench.measure<TwoSumFixture>([](TwoSumFixture& fixture) {
        Solution solution;
        return solution.twoSumBrut(fixture.nums, 579);
    });
}
