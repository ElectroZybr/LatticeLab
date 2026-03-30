#include "Profiler.h"

#include <algorithm>

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
}

void Profiler::addSample(const char* name, double ms) {
    ProfileEntry& entry = entryFor(name);
    entry.lastMs += ms;
    entry.totalMs += ms;
    entry.maxMs = std::max(entry.maxMs, ms);
    ++entry.callCount;
    ++entry.totalCallCount;
    entry.averageMs = entry.totalCallCount > 0
        ? (entry.totalMs / static_cast<double>(entry.totalCallCount))
        : 0.0;
    entry.touchedThisFrame = true;
}

ProfileEntry& Profiler::entryFor(const char* name) {
    for (ProfileEntry& entry : entries_) {
        if (entry.name == name) {
            return entry;
        }
    }

    ProfileEntry entry{};
    entry.name = name;
    entries_.push_back(entry);
    return entries_.back();
}

ProfileScope::ProfileScope(const char* name) noexcept
    : name_(name)
    , start_(Clock::now()) {}

ProfileScope::~ProfileScope() {
    const double ms = std::chrono::duration<double, std::milli>(Clock::now() - start_).count();
    Profiler::instance().addSample(name_, ms);
}
