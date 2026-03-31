#include "Profiler.h"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace {
bool profileNameEquals(const char* lhs, const char* rhs) noexcept {
    if (lhs == rhs) {
        return true;
    }
    if (lhs == nullptr || rhs == nullptr) {
        return false;
    }
    return std::string_view(lhs) == std::string_view(rhs);
}
}

Profiler& Profiler::instance() {
    static Profiler profiler;
    return profiler;
}

void Profiler::beginFrame() {
    frameStart_ = Clock::now();
    frameActive_ = true;
    frameData_.totalTrackedMs = 0.0;

    for (ProfileEntry& entry : entries_) {
        entry.lastMs = 0.0;
        entry.callCount = 0;
        entry.percentOfFrame = 0.0;
        entry.touchedThisFrame = false;
    }
}

void Profiler::endFrame() {
    if (!frameActive_) {
        return;
    }

    frameData_.frameMs = std::chrono::duration<double, std::milli>(Clock::now() - frameStart_).count();
    frameData_.totalTrackedMs = 0.0;

    for (const ProfileEntry& entry : entries_) {
        frameData_.totalTrackedMs += entry.lastMs;
    }

    const double frameMs = frameData_.frameMs;
    for (ProfileEntry& entry : entries_) {
        entry.percentOfFrame = frameMs > 0.0 ? (entry.lastMs * 100.0 / frameMs) : 0.0;
    }

    ++frameData_.frameIndex;
    frameActive_ = false;
}

void Profiler::reset() {
    frameActive_ = false;
    frameData_ = {};
    entries_.clear();
    counters_.clear();
}

void Profiler::addSample(const char* name, double ms) {
    ProfileEntry& entry = entryFor(name);
    entry.lastMs += ms;
    entry.lastActiveMs = entry.lastMs;
    entry.totalMs += ms;
    entry.maxMs = std::max(entry.maxMs, ms);
    ++entry.callCount;
    ++entry.totalCallCount;
    entry.averageMs = entry.totalCallCount > 0
        ? (entry.totalMs / static_cast<double>(entry.totalCallCount))
        : 0.0;
    entry.touchedThisFrame = true;
}

void Profiler::addCount(const char* name, size_t delta) {
    ProfileCounter& counter = counterFor(name);
    counter.pendingCount += delta;
    counter.totalCount += delta;
}

void Profiler::updateRates(double intervalSeconds) {
    if (intervalSeconds <= 0.0) {
        return;
    }

    const double alpha = 1.0 - std::exp(-intervalSeconds / 0.5);
    for (ProfileCounter& counter : counters_) {
        const double instantRate = static_cast<double>(counter.pendingCount) / intervalSeconds;
        if (!counter.hasRateSample) {
            counter.ratePerSecond = instantRate;
            counter.hasRateSample = true;
        } else {
            counter.ratePerSecond += alpha * (instantRate - counter.ratePerSecond);
        }
        counter.pendingCount = 0;
    }
}

double Profiler::counterRate(const char* name) const noexcept {
    for (const ProfileCounter& counter : counters_) {
        if (profileNameEquals(counter.name, name)) {
            return counter.ratePerSecond;
        }
    }
    return 0.0;
}

const ProfileEntry* Profiler::findEntry(const char* name) const noexcept {
    for (const ProfileEntry& entry : entries_) {
        if (profileNameEquals(entry.name, name)) {
            return &entry;
        }
    }
    return nullptr;
}

double Profiler::lastMs(const char* name) const noexcept {
    const ProfileEntry* entry = findEntry(name);
    return entry != nullptr ? entry->lastMs : 0.0;
}

double Profiler::lastActiveMs(const char* name) const noexcept {
    const ProfileEntry* entry = findEntry(name);
    return entry != nullptr ? entry->lastActiveMs : 0.0;
}

ProfileEntry& Profiler::entryFor(const char* name) {
    for (ProfileEntry& entry : entries_) {
        if (profileNameEquals(entry.name, name)) {
            return entry;
        }
    }

    ProfileEntry entry{};
    entry.name = name;
    entries_.push_back(entry);
    return entries_.back();
}

ProfileCounter& Profiler::counterFor(const char* name) {
    for (ProfileCounter& counter : counters_) {
        if (profileNameEquals(counter.name, name)) {
            return counter;
        }
    }

    ProfileCounter counter{};
    counter.name = name;
    counters_.push_back(counter);
    return counters_.back();
}

ProfileScope::ProfileScope(const char* name) noexcept
    : name_(name)
    , start_(Clock::now()) {}

ProfileScope::~ProfileScope() {
    const double ms = std::chrono::duration<double, std::milli>(Clock::now() - start_).count();
    Profiler::instance().addSample(name_, ms);
}
