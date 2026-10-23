#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <vector>

#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Kernel/TreeView.hpp>
#include <Lattice/Tools/TextFormatter.hpp>

#include "Command.hpp"
#include "Terminal.hpp"

namespace CLIPlugin {

/**
 @file CLI.hpp
 @brief Интерактивный интерфейс управления runtime Lattice

 обрабатывает подключенные терминалы, позволяет работать с экспортируемыми параметрами
 позволяет просматривать, вызывать, менять значения параметров и таблиц. 
 предоставляет навигацию по дереву компонентов.

 Инфраструктурный объект: получает runtime views и терминалы через конструктор.
 Команды, параметры темы и запуск как сервиса объявляются в композиции.
*/

class CLI final {
public:
    /// @param context node whose runtime root becomes the initial terminal path.
    CLI(ExportsView exports, Lattice::TreeView tree, Lattice::NodeId context,
        std::vector<Terminal*> terminals);

    /// Serve terminals until stopped or all terminals close. `idle` is called each poll.
    void run(const std::function<void()>& idle);
    void requestStop() { stop_ = true; }

    void showList(Lattice::ActionContext& context) const;
    void showHelp(Lattice::ActionContext& context) const;
    void showTree(Lattice::ActionContext& context) const;
    void showLogo() const;
    void changeDirectory(Lattice::ActionContext& context, std::string path) const;

    Lattice::TextTheme& theme() noexcept { return theme_; }

private:
    void broadcast(std::string_view text);
    bool pollTerminals();

    std::atomic<bool> stop_{false};
    std::vector<Terminal*> terminals_;
    CLIPlugin::CommandDispatcher commands_;
    ExportsView exports_;
    Lattice::TreeView tree_;
    Lattice::TextTheme theme_ = Lattice::TextTheme::system();
};

}
