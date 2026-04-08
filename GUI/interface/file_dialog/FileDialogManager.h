#pragma once

#include <string>

class FileDialogManager {
public:
    void openSave();
    void openLoad();
    void openCaptureDirectory(const std::string& currentPath);
    void draw(float scale);
    [[nodiscard]] bool isSaveDialogOpen() const { return saveDialogOpen_; }

private:
    bool saveDialogOpen_ = false;
};
