// Command queue without any runtime.
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <Tasks/Queue.hpp>

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

int main() {
    runsAtOnceWithoutOwner();
    ownerRunsCommandsBetweenSteps();
    std::cout << "Tasks: queue with and without an owner passed\n";
}
