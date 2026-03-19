#pragma once
#include <cstdint>
#include <string>
#include <optional>

enum class FileDialogCommand : std::uint8_t { Save, Load };

struct InterfaceCommand {
    FileDialogCommand command;
    std::string path;
};


class FileDialogManager {
public:
    void openSave();
    void openLoad();
    void draw(float scale);

    std::optional<InterfaceCommand> popResult();
private:
    std::optional<InterfaceCommand> pendingResult;
};
