#pragma once

#include <string_view>

#include <Lattice/Kernel/NodeViews.hpp>
#include <Lattice/Kernel/TreeView.hpp>

#include <CLI/include/Terminal.hpp>

namespace CLIPlugin {

enum class CommandResult {
    Continue,
    Detach
};

class CommandDispatcher {
public:
    void setExports(ExportsView exports) { exports_ = exports; }
    void setTree(Lattice::TreeView tree) { tree_ = tree; }
    CommandResult execute(Terminal& terminal, std::string_view command) const;

private:
    ExportsView exports_;
    Lattice::TreeView tree_;
};

}
