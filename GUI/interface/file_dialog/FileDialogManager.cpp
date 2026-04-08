#include "FileDialogManager.h"

#include <filesystem>

#include <ImGuiFileDialog.h>
#include <imgui.h>

#include "App/AppSignals.h"

namespace {
    std::string defaultSimulationPath() {
        constexpr const char* demoScenesPath = "demo/scenes";
        if (std::filesystem::exists(demoScenesPath)) {
            return demoScenesPath;
        }
        return ".";
    }
}

void FileDialogManager::openSave() {
    IGFD::FileDialogConfig config;
    config.path = defaultSimulationPath();
    config.fileName = "scene";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
    ImGuiFileDialog::Instance()->OpenDialog("SaveDlg", "Save scene", ".lat", config);
}

void FileDialogManager::openLoad() {
    IGFD::FileDialogConfig config;
    config.path = defaultSimulationPath();
    config.countSelectionMax = 1;
    ImGuiFileDialog::Instance()->OpenDialog("LoadDlg", "Load scene", ".lat,.sim", config);
}

void FileDialogManager::openCaptureDirectory(const std::string& currentPath) {
    IGFD::FileDialogConfig config;
    config.path = currentPath.empty() ? "." : currentPath;
    config.countSelectionMax = 1;
    ImGuiFileDialog::Instance()->OpenDialog("CaptureDirDlg", "Select capture directory", nullptr, config);
}

void FileDialogManager::draw(float scale) {
    ImVec2 dlgSize(400 * scale, 300 * scale);

    if (ImGuiFileDialog::Instance()->Display("SaveDlg", ImGuiWindowFlags_NoCollapse, dlgSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            AppSignals::UI::SaveSimulation.emit(ImGuiFileDialog::Instance()->GetFilePathName());
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("LoadDlg", ImGuiWindowFlags_NoCollapse, dlgSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            AppSignals::UI::LoadSimulation.emit(ImGuiFileDialog::Instance()->GetFilePathName());
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("CaptureDirDlg", ImGuiWindowFlags_NoCollapse, dlgSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            AppSignals::Capture::SetOutputDirectory.emit(ImGuiFileDialog::Instance()->GetCurrentPath());
        }
        ImGuiFileDialog::Instance()->Close();
    }
}
