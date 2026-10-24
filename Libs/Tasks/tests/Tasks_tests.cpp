// Command queue without any runtime.
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <Tasks/Pool.hpp>
#include <Tasks/Queue.hpp>
#include <numeric>
#include <set>
#include <vector>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void runsAtOnceWithoutOwner() {
    Tasks::Queue queue;
    int value = 0;
    queue.post([&] { value = 1; });
    check(value == 1, "no owner: the command runs on the caller");
    check(queue.call([&] { return value + 1; }) == 2, "call returns the result");
}

void ownerRunsCommandsBetweenSteps() {
    Tasks::Queue queue;
    std::atomic<bool> stop{false};
    std::thread::id ranOn;
    int steps = 0, seen = -1;
    std::thread owner([&] {
        queue.beginServing();
        while (!stop) {
            queue.runUntil(std::chrono::steady_clock::now() + std::chrono::milliseconds(5), [&] { return stop.load(); });
            ++steps;   // owner's own work; commands never overlap it
        }
        queue.endServing();
    });
    while (!queue.serving())
        std::this_thread::yield();
    queue.post([&] { ranOn = std::this_thread::get_id(); });
    seen = queue.call([&] { return steps; });
    check(ranOn == owner.get_id(), "commands run on the owner thread");
    check(seen >= 0, "call waits for the owner");
    stop = true;
    queue.wake();
    owner.join();

    int late = 0;
    queue.post([&] { late = 1; });
    check(late == 1, "after serving ends commands run on the caller");
}
}

void poolCoversEveryRowOnce() {
    Tasks::Pool pool{4};
    check(pool.threads() == 4, "threads include the caller");
    for (size_t rows : {size_t(1), size_t(63), size_t(64), size_t(1000), size_t(100'003)})
        for (size_t workers : {1, 2, 3, 4, 8}) {
            std::vector<int> seen(rows, 0);
            std::mutex mutex;
            std::set<std::thread::id> threads;
            pool.forPartitions(rows, workers, [&](size_t begin, size_t end) {
                check(begin < end && end <= rows, "a partition is a range of rows");
                check(begin % 64 == 0, "partitions start on a grain");
                for (size_t i = begin; i < end; ++i)
                    ++seen[i];
                std::lock_guard lock(mutex);
                threads.insert(std::this_thread::get_id());
            });
            check(std::ranges::all_of(seen, [](int v) { return v == 1; }), "every row once");
            check(threads.size() <= std::min<size_t>(workers, 4), "no more threads than workers");
            if (rows >= 1000 && workers >= 2)
                check(threads.size() >= 2, "large jobs run in parallel");
        }
    // Many small jobs in a row (the pool is reused), and errors reach the caller.
    std::atomic<size_t> total = 0;
    for (int i = 0; i < 1000; ++i)
        pool.forPartitions(4096, 4, [&](size_t begin, size_t end) { total += end - begin; });
    check(total == 4096 * 1000, "reused for many jobs");
    bool thrown = false;
    try {
        pool.forPartitions(4096, 4, [](size_t begin, size_t) {
            if (begin > 0) throw std::runtime_error("kernel failed");
        });
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    check(thrown, "a kernel's exception is rethrown");
}

int main() {
    runsAtOnceWithoutOwner();
    ownerRunsCommandsBetweenSteps();
    poolCoversEveryRowOnce();
    std::cout << "Tasks: queue with and without an owner, pool of partitions passed\n";
}
