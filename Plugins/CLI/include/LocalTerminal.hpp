#pragma once

#include <memory>

#include <Lattice/Kernel/NodeViews.hpp>

#include <CLI/include/Terminal.hpp>

namespace CLIPlugin {

class LocalTerminal final : public Terminal {
public:
    explicit LocalTerminal(NodeBuild);
    ~LocalTerminal() override;

protected:
    void onAttach() override;
    void onDetach() override;
    Input onPoll() override;
    void onWrite(std::string_view text) override;
    void onPathChanged(std::string path) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
