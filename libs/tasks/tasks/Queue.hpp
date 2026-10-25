#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

namespace Tasks {

/**
 * Commands for the thread that owns some state (a simulation loop).
 *
 * While the owner serves the queue, post() hands a command to it and returns; the
 * owner runs commands between its own steps (runUntil). While nobody serves, a
 * command runs at once on the caller's thread, one at a time. Either way commands
 * never run concurrently with each other or with the owner's work.
 */
class Queue final {
public:
    using Task = std::function<void()>;

    /// Run `task` on the owner thread (later), or now when nobody serves.
    void post(Task task) {
        std::unique_lock lock(mutex_);
        if (!serving_ || std::this_thread::get_id() == owner_) {
            lock.unlock();
            std::lock_guard run(running_);
            task();
            return;
        }
        tasks_.push_back(std::move(task));
        wake_.notify_all();
    }

    /// Run `task` like post() and wait for its result.
    template<class F>
    auto call(F&& task) -> std::invoke_result_t<F&> {
        using Result = std::invoke_result_t<F&>;
        std::packaged_task<Result()> packaged(std::forward<F>(task));
        auto result = packaged.get_future();
        post([&packaged] { packaged(); });
        return result.get();
    }

    // Owner side.

    /// The calling thread starts serving: post() now hands commands to it.
    void beginServing() {
        std::lock_guard lock(mutex_);
        owner_ = std::this_thread::get_id();
        serving_ = true;
    }

    /// Stop serving; commands still queued run now, later ones run on their callers.
    void endServing() {
        {
            std::lock_guard lock(mutex_);
            serving_ = false;
            owner_ = {};
        }
        runPending();
    }

    /// Run arriving commands until `deadline` or until `stop()` turns true.
    template<class Stop>
    void runUntil(std::chrono::steady_clock::time_point deadline, Stop&& stop) {
        while (!stop()) {
            runPending();
            std::unique_lock lock(mutex_);
            if (!wake_.wait_until(lock, deadline, [&] { return !tasks_.empty() || stop(); }))
                return;
        }
    }

    bool serving() const {
        std::lock_guard lock(mutex_);
        return serving_;
    }

    /// Wake a waiting owner (after changing what its stop condition reads).
    void wake() {
        std::lock_guard lock(mutex_);
        wake_.notify_all();
    }

private:
    void runPending() {
        std::deque<Task> tasks;
        {
            std::lock_guard lock(mutex_);
            tasks.swap(tasks_);
        }
        std::lock_guard run(running_);
        for (auto& task : tasks)
            task();
    }

    mutable std::mutex mutex_;
    // Serializes commands run off the owner thread with each other and with queued ones.
    std::recursive_mutex running_;
    std::condition_variable wake_;
    std::deque<Task> tasks_;
    std::thread::id owner_;
    bool serving_ = false;
};

}
