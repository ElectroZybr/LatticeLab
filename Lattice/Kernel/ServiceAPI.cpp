#include <Lattice/Kernel/ServiceAPI.hpp>

#include <Lattice/Kernel/NodeOps.hpp>

void ServiceAPI::enter(Lattice::NodeOps& hostOps) {
    if (running_.exchange(true))
        return;

    host_ = true;
    hostOps_ = &hostOps;

    try {
        run();
    } catch (...) {
        hostOps_ = nullptr;
        running_ = false;
        throw;
    }

    hostOps_ = nullptr;
    running_ = false;
}

bool ServiceAPI::stopRequested() const {
    if (hostOps_)
        hostOps_->maintain();

    return stopRequested_.load();
}
