#pragma once

#include <string_view>

#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Kernel/TreeView.hpp>

#include "Terminal.hpp"

namespace CLIPlugin {

class CommandDispatcher {
    ExportsView exports_;
    Lattice::TreeView tree_;
    const Lattice::TextTheme* theme_ = &Lattice::TextTheme::system();
    void reply(Terminal&, Level, std::string_view) const;
    void reply(Terminal&, std::string_view) const;
    
public:
    void setTheme(const Lattice::TextTheme& theme) { theme_ = &theme; }
    void setExports(ExportsView exports) { exports_ = exports; }
    void setTree(Lattice::TreeView tree) { tree_ = tree; }
    void execute(Terminal& terminal, std::string_view command) const;
    ExportsView getExports() { return exports_; }
};

}
