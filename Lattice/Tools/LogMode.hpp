#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include <Lattice/Kernel/Exception.hpp>


enum class Level : uint8_t {
    Message,
    Ok,
    Action,
    Info,
    Warning,
    Error,
    Exception,
    Blank
};


enum class LogMode : uint8_t {
    Verbose       = 1 << 0,
    OnlyWarn      = 1 << 1,
    Clean         = 1 << 2,
    SuppressError = 1 << 3,
    Gap           = 1 << 4,
};


inline constexpr LogMode operator|(LogMode lhs, LogMode rhs) noexcept {
    return static_cast<LogMode>(
        static_cast<uint8_t>(lhs) |
        static_cast<uint8_t>(rhs)
    );
}

inline constexpr LogMode operator&(LogMode lhs, LogMode rhs) noexcept {
    return static_cast<LogMode>(
        static_cast<uint8_t>(lhs) &
        static_cast<uint8_t>(rhs)
    );
}

inline constexpr LogMode operator~(LogMode mode) noexcept {
    return static_cast<LogMode>(
        ~static_cast<uint8_t>(mode)
    );
}

inline constexpr LogMode& operator|=(LogMode& lhs, LogMode rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

inline constexpr LogMode& operator&=(LogMode& lhs, LogMode rhs) noexcept {
    lhs = lhs & rhs;
    return lhs;
}

inline constexpr bool hasMode(LogMode value, LogMode mode) noexcept {
    return (value & mode) != static_cast<LogMode>(0);
}


namespace LogModes {

inline constexpr size_t UnlimitedDepth = std::numeric_limits<size_t>::max();

inline constexpr LogMode Default =
    LogMode::Clean | LogMode::OnlyWarn | LogMode::Gap;

inline constexpr LogMode Inheritable =
    LogMode::Verbose | LogMode::OnlyWarn | LogMode::Gap;


constexpr LogMode inherit(LogMode local, LogMode parent) noexcept {
    const bool verbose =
        hasMode(local, LogMode::Verbose) ||
        hasMode(parent, LogMode::Verbose);
    const bool onlyWarn =
        hasMode(local, LogMode::OnlyWarn) ||
        hasMode(parent, LogMode::OnlyWarn);
    const bool gap =
        hasMode(local, LogMode::Gap) ||
        hasMode(parent, LogMode::Gap);

    if (verbose) {
        LogMode mode = LogMode::Verbose;
        if (onlyWarn)
            mode |= LogMode::OnlyWarn;
        if (gap)
            mode |= LogMode::Gap;
        return mode;
    }

    if (onlyWarn && !hasMode(local, LogMode::SuppressError))
        local |= LogMode::OnlyWarn;

    if (gap)
        local |= LogMode::Gap;

    return local;
}

constexpr bool isError(Level level) noexcept {
    return
        level == Level::Error ||
        level == Level::Exception;
}

constexpr bool isNoise(Level level) noexcept {
    return
        level == Level::Message ||
        level == Level::Info;
}


struct KeepContext {
    LogMode mode = Default;
    Level level = Level::Message;
    bool success = true;
    size_t depth = 1;
    size_t maxDepth = UnlimitedDepth;
    bool isCurrentFinal = false;
    bool isScopeFinal = false;
    bool hadProblem = false;
};


constexpr bool shouldKeepGap(LogMode mode, bool keptAction) noexcept {
    return keptAction && hasMode(mode, LogMode::Gap);
}


constexpr bool shouldKeep(const KeepContext& q) noexcept {
    const bool verbose       = hasMode(q.mode, LogMode::Verbose);
    const bool onlyWarn      = hasMode(q.mode, LogMode::OnlyWarn);
    const bool clean         = hasMode(q.mode, LogMode::Clean);
    const bool suppressError = hasMode(q.mode, LogMode::SuppressError);

    if (q.level == Level::Blank)
        return false;

    if (suppressError && isError(q.level))
        return false;

    if (q.isCurrentFinal) {
        if (q.success)
            return q.level == Level::Ok;
        return isError(q.level);
    }

    if (verbose) {
        if (onlyWarn)
            return !isNoise(q.level);
        return true;
    }

    if (clean && !onlyWarn)
        return false;

    if (!isError(q.level) && q.depth >= q.maxDepth)
        return false;

    if (onlyWarn) {
        if (isNoise(q.level))
            return false;

        if (q.level == Level::Ok)
            return !clean || q.isScopeFinal;

        if (q.level == Level::Action)
            return !clean || q.hadProblem || !q.success;

        return true;
    }

    return true;
}


constexpr bool shouldAnnotateWarn(
    LogMode mode,
    bool success,
    bool hadProblem
) noexcept {
    if (!success || !hadProblem)
        return false;

    if (hasMode(mode, LogMode::SuppressError))
        return false;

    if (hasMode(mode, LogMode::Verbose) || hasMode(mode, LogMode::OnlyWarn))
        return false;

    return hasMode(mode, LogMode::Clean);
}

}
