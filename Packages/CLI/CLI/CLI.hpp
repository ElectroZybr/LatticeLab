#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Kernel/TreeView.hpp>
#include <Lattice/Tools/TextFormatter.hpp>

#include "Command.hpp"
#include "Terminal.hpp"

namespace Console {

/**
 @file CLI.hpp
 @brief Интерактивный интерфейс управления runtime Lattice

 обрабатывает подключенные терминалы, позволяет работать с экспортируемыми параметрами
 позволяет просматривать, вызывать, менять значения параметров и таблиц. 
 предоставляет навигацию по дереву компонентов.

 Инфраструктурный объект: получает runtime views через конструктор, а текущий набор
 терминалов — от композиции (setTerminals в configure). Команды, параметры темы и
 запуск как сервиса объявляются в композиции.

 Терминалы обслуживает поток CLI. Строки и ответы передаются без ожидания;
 processCommands выполняется только на host, не обращаясь к физическим терминалам.
*/

class CLI final {
public:
    /// @param context node whose runtime root becomes the initial terminal path.
    CLI(ExportsView exports, Lattice::TreeView tree, Lattice::NodeId context);

    /// Host: replace the terminal set. Removed sessions and their pending work are
    /// cancelled before returning; new terminals attach on the I/O thread.
    void setTerminals(std::vector<Terminal*> terminals);

    /// Serve terminals until stopped or all terminals close. `idle` is called each poll.
    void run(const std::function<void()>& idle);
    void requestStop() { stop_ = true; }
    /// Host checkpoint: execute queued lines, returning replies to the I/O thread.
    void processCommands();

    void showList(Lattice::ActionContext& context) const;
    void showHelp(Lattice::ActionContext& context) const;
    void showTree(Lattice::ActionContext& context) const;
    /// Subtree of params and actions: only nodes that own exports or lead to them.
    void showExports(Lattice::ActionContext& context) const;
    void showLogo() const;
    void changeDirectory(Lattice::ActionContext& context, std::string path) const;

    Lattice::TextTheme& theme() noexcept { return theme_; }

private:
    bool pollTerminals();

    std::atomic<bool> stop_{false};
    struct Reply {
        std::string text;
        Lattice::NodeId current = Lattice::InvalidNodeId;
        std::string path;
    };
    struct Session {
        uint64_t id;
        Terminal* terminal; // Borrowed only under mutex_; requests carry the id.
        Lattice::NodeId current;
        bool started = false;
        bool closed = false;
        size_t pending = 0;
        std::deque<Reply> replies;
    };
    struct Request {
        uint64_t session;
        std::string line;
        bool startup = false;
    };
    std::mutex mutex_;
    std::vector<Session> sessions_;
    std::deque<Request> requests_;
    uint64_t nextSession_ = 1;
    bool processing_ = false; // host only: a command may enter maintain recursively.
    Lattice::NodeId root_ = Lattice::InvalidNodeId;
    std::string rootPath_;
    Console::CommandDispatcher commands_;
    ExportsView exports_;
    Lattice::TreeView tree_;
    // The shared system theme: its exported styles recolor every output, not a copy.
    Lattice::TextTheme& theme_ = Lattice::TextTheme::system();
};

}
