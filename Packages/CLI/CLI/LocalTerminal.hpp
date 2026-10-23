#pragma once

#include <memory>

#include "Terminal.hpp"

namespace Console {

class LocalTerminal final : public Terminal {
public:
    LocalTerminal();
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
