#pragma once

#include <atomic>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <Lattice/kernel/Consts.hpp>
#include "TerminalStyle.hpp"

namespace Console {

/**
 @file Terminal.hpp
 @brief Базовый интерфейс терминала.

 Terminal представляет подключаемую консоль. Хранит состояние подключения,
 текущую ноду и параметры отображения. Экспорт параметров — в композиции.
*/

class Terminal {
    std::atomic<bool> attached_{false};
    Lattice::NodeId current_ = Lattice::InvalidNodeId;
    TerminalStyle style_;

public:
    Terminal() = default;

    const TerminalStyle& style() const noexcept { return style_; }
    TerminalStyle& style() noexcept { return style_; }

    struct Input {
        std::optional<std::string> line;
        bool closed = false;
    };

    virtual ~Terminal() = default;

    void attach() {
        if (!attached_.exchange(true))
            onAttach();
    }

    void detach() {
        if (attached_.exchange(false))
            onDetach();
    }

    bool attached() const noexcept { return attached_.load(); }

    Lattice::NodeId current() const noexcept { return current_; }

    void setCurrent(Lattice::NodeId current, std::string path) {
        current_ = current;
        onPathChanged(std::move(path));
    }

    // poll() must be non-blocking so one CLI loop can serve many terminals.
    Input poll() { return attached() ? onPoll() : Input{}; }

    void write(std::string_view text) {
        if (attached())
            onWrite(text);
    }

protected:
    virtual void onAttach() = 0;
    virtual void onDetach() = 0;
    virtual Input onPoll() = 0;
    virtual void onWrite(std::string_view text) = 0;
    virtual void onPathChanged(std::string path) = 0;
};

}
