#pragma once
#include <optional>
#include <string>
#include <cstdint>

enum class SimCommand : std::uint8_t { Save, Load };

struct FileDialogResult {
    SimCommand command;
    std::string path;
};

class FileDialogManager {
public:
    void openSave();
    void openLoad();
    void draw(float scale);

    std::optional<FileDialogResult> popResult();
private:
    std::optional<FileDialogResult> pendingResult;
};
