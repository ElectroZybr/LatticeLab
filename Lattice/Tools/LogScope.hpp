#pragma once

#include <chrono>
#include <string>
#include <string_view>

#include <Lattice/Tools/Logger.hpp>


class LogScope {
public:
    template <typename... Args>
    LogScope(std::string_view tag, std::format_string<Args...> format, Args&&... args)
        : tag_(tag)
        , finishMessage_("Done")
        , startTime_(Clock::now())
    {
        auto& log = LogSystem::current();
        start(log.currentOrDefault(), log.defaultMaxDepth(),
              Text::format(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    LogScope(std::string_view tag, LogMode mode, std::format_string<Args...> format, Args&&... args)
        : tag_(tag)
        , finishMessage_("Done")
        , startTime_(Clock::now())
    {
        auto& log = LogSystem::current();
        start(
            LogModes::inherit(mode, log.currentOrDefault()),
            log.defaultMaxDepth(),
            Text::format(format, std::forward<Args>(args)...)
        );
    }

    template <typename... Args>
    LogScope(
        std::string_view tag,
        LogMode mode,
        size_t maxDepth,
        std::format_string<Args...> format,
        Args&&... args
    )
        : tag_(tag)
        , finishMessage_("Done")
        , startTime_(Clock::now())
    {
        auto& log = LogSystem::current();
        start(
            LogModes::inherit(mode, log.currentOrDefault()),
            maxDepth,
            Text::format(format, std::forward<Args>(args)...)
        );
    }

    ~LogScope() {
        if (active_)
            finishError("aborted");
    }

    void finish() noexcept {
        finish("{}", finishMessage_);
    }

    template <typename... Args>
    void finish(std::format_string<Args...> format, Args&&... args) {
        if (!active_)
            return;

        close(Level::Ok, Text::format(format, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void finishError(std::format_string<Args...> format, Args&&... args) {
        if (!active_)
            return;

        close(Level::Exception, Text::format(format, std::forward<Args>(args)...));
    }

    void cancel() noexcept {
        if (!active_)
            return;

        closeWithoutMessage();
    }

private:
    using Clock = std::chrono::steady_clock;

    void start(LogMode mode, size_t maxDepth, const Text& message) {
        auto& log = LogSystem::current();
        log.pushScope(mode, maxDepth);
        Logger::print(Level::Action, tag_, message);
        log.addDepth(1);
    }

    void close(Level level, const Text& message) {
        auto& log = LogSystem::current();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - startTime_
        ).count();

        Text finish = message;
        if (LogModes::shouldAnnotateWarn(
                log.currentScopeMode(),
                level == Level::Ok,
                log.currentHadProblem()
            ))
        {
            finish += Text::format(" <y>(warn)</>");
        }
        finish += Text::format("<gr> ({} us)</>", elapsed);

        log.addDepth(-1);
        Logger::print(level, tag_, finish, true);
        log.popScope(level == Level::Ok);

        active_ = false;
    }

    void closeWithoutMessage() noexcept {
        auto& log = LogSystem::current();
        log.addDepth(-1);
        log.popScope(false, false);
        active_ = false;
    }

    std::string tag_;
    std::string finishMessage_;
    Clock::time_point startTime_;
    bool active_ = true;
};
