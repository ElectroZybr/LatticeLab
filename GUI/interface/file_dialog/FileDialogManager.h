#pragma once

#include <string>

class FileDialogManager {
public:
    void openSave();
    void openLoad();
    void openCaptureDirectory(const std::string& currentPath);
    void draw(float scale);
};
