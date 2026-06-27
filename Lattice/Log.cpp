#include "Lattice/Log.hpp"

#include <atomic>

namespace {
std::atomic_bool gLogVerbose = false;
}

void Log::setVerbose(bool enabled) noexcept {
    gLogVerbose.store(enabled, std::memory_order_relaxed);
}

bool Log::isVerbose() noexcept {
    return gLogVerbose.load(std::memory_order_relaxed);
}

std::mutex& Log::mutex() {
    static std::mutex consoleMutex;
    return consoleMutex;
}

void Log::print(std::ostream& stream, std::string_view status, std::string_view color, std::string_view tag, const std::string& message) {
    std::lock_guard lock(mutex());
    stream << color << status << LogStyle::Color::reset
           << " ["
           << LogStyle::Color::bold
           << tag
           << LogStyle::Color::reset
           << "] "
           << LogStyle::Color::tree
           << message
           << LogStyle::Color::reset
           << '\n';
}
