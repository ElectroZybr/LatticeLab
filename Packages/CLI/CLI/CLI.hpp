#pragma once

#include <atomic>
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

 Терминалы обслуживает поток CLI, а набор меняется из host-потока: setTerminals ждёт
 конца текущей итерации, после возврата убранные терминалы больше не используются.
 Команда CLI поэтому не должна ждать host-поток.
*/

class CLI final {
public:
    /// @param context node whose runtime root becomes the initial terminal path.
    CLI(ExportsView exports, Lattice::TreeView tree, Lattice::NodeId context);

    /// Serve exactly these terminals: new ones start at the runtime root (and attach
    /// if serving), removed ones detach.
    void setTerminals(std::vector<Terminal*> terminals);

    /// Serve terminals until stopped or all terminals close. `idle` is called each poll.
    void run(const std::function<void()>& idle);
    void requestStop() { stop_ = true; }

    void showList(Lattice::ActionContext& context) const;
    void showHelp(Lattice::ActionContext& context) const;
    void showTree(Lattice::ActionContext& context) const;
    /// Subtree of params and actions: only nodes that own exports or lead to them.
    void showExports(Lattice::ActionContext& context) const;
    void showLogo() const;
    void changeDirectory(Lattice::ActionContext& context, std::string path) const;

    Lattice::TextTheme& theme() noexcept { return theme_; }

private:
    void broadcast(std::string_view text);
    bool pollTerminals();

    std::atomic<bool> stop_{false};
    bool serving_ = false;
    // Recursive: commands and logging on the CLI thread write back to terminals.
    std::recursive_mutex mutex_;
    std::vector<Terminal*> terminals_;
    Lattice::NodeId root_ = Lattice::InvalidNodeId;
    Console::CommandDispatcher commands_;
    ExportsView exports_;
    Lattice::TreeView tree_;
    // The shared system theme: its exported styles recolor every output, not a copy.
    Lattice::TextTheme& theme_ = Lattice::TextTheme::system();
};

}
