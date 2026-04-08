#pragma once

#include <string>

class FileDialogManager {
public:
    void openSave();
    void openLoad();
    void openCaptureDirectory(const std::string& currentPath);
    void openSceneDirectory(const std::string& currentPath);
    void draw(float scale);
    [[nodiscard]] bool isSaveDialogOpen() const { return saveDialogOpen_; }
    [[nodiscard]] bool hasSelectedSceneDirectory() const { return sceneDirectorySelected_; }
    std::string consumeSelectedSceneDirectory();

private:
    bool saveDialogOpen_ = false;
    bool sceneDirectorySelected_ = false;
    std::string selectedSceneDirectory_;
};
