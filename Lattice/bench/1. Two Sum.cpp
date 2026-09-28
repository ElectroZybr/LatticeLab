#include <unordered_map>
#include <vector>

#include <Lattice/Tools/Benchmark.hpp>

using namespace Lattice;
using namespace std;

class Solution {
public:
    vector<int> twoSumHash(vector<int>& nums, int target) {
        unordered_map<int, int> seen;
        for (int i = 0; i < nums.size(); i++) {
            int need = target - nums[i];
            if (seen.contains(need))
                return {seen[need], i};
            seen[nums[i]] = i;
        }
        return {};
    }

    vector<int> twoSumBrut(vector<int>& nums, int target) {
        for (int i = 0; i < nums.size(); i++)
            for (int j = i + 1; j < nums.size(); j++)
                if (nums[i] + nums[j] == target && i != j)
                    return {i, j};
        return {};
    }
};

BENCH(TwoSum, Hash, "unordered_map implementation") {
    bench.config.sizes = {10, 100, 1000, 10000};
    bench.config.warmup = 32;
    bench.config.samples = 20;
    bench.config.target = 50ms;

    bench.measure(
        [](size_t n) {
            vector<int> nums(n, 1);
            nums[n - 2] = 123;
            nums[n - 1] = 456;
            return nums;
        },
        [](vector<int>& nums) {
            Solution solution;
            return solution.twoSumHash(nums, 579);
        }
    );
}

BENCH(TwoSum, Brut, "unordered_map implementation") {
    bench.config.sizes = {10, 100, 1000, 10000};
    bench.config.warmup = 32;
    bench.config.samples = 20;
    bench.config.target = 50ms;

    bench.measure(
        [](size_t n) {
            vector<int> nums(n, 1);
            nums[n - 2] = 123;
            nums[n - 1] = 456;
            return nums;
        },
        [](vector<int>& nums) {
            Solution solution;
            return solution.twoSumBrut(nums, 579);
        }
    );
}

