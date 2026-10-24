#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace Tasks {

/// Threads that run the partitions of one job at a time: a kernel over rows
/// [begin, end) — the shape every CPU kernel has ("kernel per partition"). The caller
/// runs a partition itself; the pool's threads run the rest. One job at a time: jobs
/// from several threads are serialized.
class Pool final {
public:
    /// `threads` in all, the caller included (the pool starts threads − 1).
    explicit Pool(size_t threads = std::max(1u, std::thread::hardware_concurrency())) {
        for (size_t i = 1; i < threads; ++i)
            threads_.emplace_back([this, i] { serve(i); });
    }

    ~Pool() {
        {
            std::lock_guard lock(mutex_);
            stop_ = true;
            ++generation_;
        }
        wake_.notify_all();
        for (auto& thread : threads_)
            thread.join();
    }

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    size_t threads() const noexcept { return threads_.size() + 1; }

    /// Split [0, rows) into at most `workers` contiguous partitions (whole multiples of
    /// `grain` rows but the last) and run `kernel(begin, end)` on them in parallel;
    /// returns when all are done. The first exception of a kernel is rethrown here.
    template<class Kernel>
    void forPartitions(size_t rows, size_t workers, Kernel&& kernel, size_t grain = 64) {
        if (rows == 0)
            return;
        workers = std::clamp<size_t>(workers, 1, threads());
        const size_t grains = (rows + grain - 1) / grain;
        const size_t chunk = ((grains + workers - 1) / workers) * grain;
        const size_t parts = (rows + chunk - 1) / chunk;
        if (parts == 1) {
            kernel(size_t(0), rows);
            return;
        }

        std::lock_guard serial(job_);
        auto run = [&](size_t part) {
            const size_t begin = part * chunk;
            const size_t end = std::min(rows, begin + chunk);
            try {
                kernel(begin, end);
            } catch (...) {
                std::lock_guard lock(mutex_);
                if (!error_)
                    error_ = std::current_exception();
            }
        };
        {
            std::lock_guard lock(mutex_);
            task_ = run;
            parts_ = parts;
            remaining_ = parts - 1;
            error_ = nullptr;
            ++generation_;
        }
        wake_.notify_all();
        run(0);
        {
            std::unique_lock lock(mutex_);
            done_.wait(lock, [&] { return remaining_ == 0; });
            task_ = nullptr;
        }
        if (error_)
            std::rethrow_exception(std::exchange(error_, nullptr));
    }

private:
    void serve(size_t index) {
        uint64_t seen = 0;
        while (true) {
            std::function<void(size_t)> task;
            {
                std::unique_lock lock(mutex_);
                wake_.wait(lock, [&] { return generation_ != seen; });
                seen = generation_;
                if (stop_)
                    return;
                if (index >= parts_)
                    continue;   // fewer partitions than threads: this one sits out
                task = task_;
            }
            task(index);
            {
                std::lock_guard lock(mutex_);
                if (--remaining_ == 0)
                    done_.notify_one();
            }
        }
    }

    std::vector<std::thread> threads_;
    std::mutex job_;     // one job at a time
    std::mutex mutex_;
    std::condition_variable wake_, done_;
    std::function<void(size_t)> task_;
    size_t parts_ = 0;
    size_t remaining_ = 0;
    uint64_t generation_ = 0;
    bool stop_ = false;
    std::exception_ptr error_;
};

}
