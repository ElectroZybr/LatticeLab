#include "FileDialogManager.h"

#include <ImGuiFileDialog.h>
#include <imgui.h>

#include "App/AppSignals.h"

void FileDialogManager::openSave() {
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.fileName = "simulation";
    config.countSelectionMax = 1;
    config.flags = ImGuiFileDialogFlags_ConfirmOverwrite;
    ImGuiFileDialog::Instance()->OpenDialog("SaveDlg", "Save simulation", ".sim", config);
}

void FileDialogManager::openLoad() {
    IGFD::FileDialogConfig config;
    config.path = ".";
    config.countSelectionMax = 1;
    ImGuiFileDialog::Instance()->OpenDialog("LoadDlg", "Load simulation", ".sim", config);
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
}
