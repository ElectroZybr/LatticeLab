#pragma once

#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Kernel/ServiceAPI.hpp>
#include <Lattice/Kernel/TreeView.hpp>
#include <Lattice/Tools/TextFormatter.hpp>

#include <CLI/include/Command.hpp>
#include <CLI/include/Terminal.hpp>

namespace CLIPlugin {

/**
 @file CLI.hpp
 @brief Интерактивный интерфейс управления runtime Lattice

 обрабатывает подключенные терминалы, позволяет работать с экспортируемыми параметрами
 позволяет просматривать, вызывать, менять значения параметров и таблиц. 
 предоставляет навигацию по дереву компонентов
*/

class CLI final : public ServiceAPI {
public:
    explicit CLI(NodeBuild branch);
    void configure(NodeConfigure branch);

private:
    void run() override;
    void broadcast(std::string_view text);
    bool pollTerminals();
    void showList(Lattice::ActionContext& context) const;
    void showHelp(Lattice::ActionContext& context) const;
    void showTree(Lattice::ActionContext& context) const;
    void showLogo() const;
    
    void changeDirectory(Lattice::ActionContext& context, std::string path) const;

    Children<CLIPlugin::Terminal> terminals_;
    CLIPlugin::CommandDispatcher commands_;
    ExportsView exports_;
    Lattice::TreeView tree_;
    Lattice::TextTheme theme_ = Lattice::TextTheme::system();
};

}
