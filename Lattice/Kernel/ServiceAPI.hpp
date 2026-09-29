#pragma once

#include <Lattice/Kernel/Consts.hpp>

#include <thread>
#include <atomic>

namespace Lattice {
class NodeOps;
}

enum class ServiceLaunch {
    Worker,
    Host
};

struct ServiceAPI : public Lattice::Component {
    ServiceAPI() = default;
    ServiceAPI(const ServiceAPI&) = delete;
    ServiceAPI& operator=(const ServiceAPI&) = delete;
    ServiceAPI& operator=(ServiceAPI&&) = delete;

    void start() {
        if (running_.exchange(true))
            return;

        thread_ = std::thread([this] {
            run();
            running_ = false;
        });
    }

    void enter(Lattice::NodeOps& hostOps);

    void stop() {
        requestStop();
        if (thread_.joinable())
            thread_.join();
        running_ = false;
    }

    bool running() const { return running_.load(); }
    bool host() const { return host_; }

    void retire() {
        requestStop();
    }

    bool readyToDestroy() const {
        return !running();
    }

    virtual ~ServiceAPI() {
        stop();
    }

protected:
    virtual void run() = 0;

    virtual void requestStop() {
        stopRequested_ = true;
    }

    bool stopRequested() const;

private:
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
    Lattice::NodeOps* hostOps_ = nullptr;
    bool host_ = false;
};
