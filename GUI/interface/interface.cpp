#include "interface.h"

#include "App/capture/CaptureController.h"
#include "imgui_impl_opengl3.h"

#include <SFML/Graphics.hpp>
#define ICON_MIN_FA 0xf000
#define ICON_MAX_FA 0xf897

#define ICON_FA_FLASK "\uf0c3"
#define ICON_FA_COG "\uf013"
#define ICON_FA_PAUSE "\uf04c"
#define ICON_FA_PLAY "\uf04b"
#define ICON_FA_FORWARD "\uf04e"
#define ICON_FA_BACKWARD "\uf04a"
#define ICON_FA_FAST_FORWARD "\uf050"
#define ICON_FA_FAST_BACKWARD "\uf049"
#define ICON_FA_BUG "\uf188"

Interface::Interface(sf::RenderWindow& w, Simulation& s, std::unique_ptr<IRenderer>& r, CaptureController& c)
    : window_(&w), simulation_(&s), renderer_(&r), captureController_(&c) {}

int Interface::init() {
    if (!ImGui::SFML::Init(*window_, false)) {
        return EXIT_FAILURE;
    }

    styleManager.applyCustomStyle();

    if (!fontManager.load(styleManager.getScale())) {
        return EXIT_FAILURE;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 150")) {
        return EXIT_FAILURE;
    }
    if (!ImGui_ImplOpenGL3_CreateFontsTexture()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void Interface::shutdown() {
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::SFML::Shutdown();
}

int Interface::update() {
    ImGui_ImplOpenGL3_NewFrame();
    const sf::Time delta = clock_.restart();
    ImGui::SFML::Update(*window_, delta);

    ImGui::PushFont(fontManager.main);
    toolsPanel.draw(styleManager.getScale(), *window_, debugPanel, settingsPanel, ioPanel);
    periodicPanel.draw(styleManager.getScale(), window_->getSize(), uiState_.selectedAtom);
    simControlPanel.draw(styleManager.getScale(), window_->getSize(), uiState_.pause, uiState_.simulationSpeed, uiState_.simStep,
                         delta.asSeconds());
    sideToolsPanel.draw(styleManager.getScale(), window_->getSize(), fontManager.icons, fontManager.dialog);
    statsPanel.draw(styleManager.getScale(), window_->getSize());
    if (uiState_.drawToolTrip) {
        const ImVec2 mouse = ImGui::GetMousePos();
        ImGui::SetNextWindowPos(ImVec2(mouse.x + 3 * styleManager.getScale(), mouse.y + 3 * styleManager.getScale()));

        ImGui::BeginTooltip();
        if (!uiState_.toolTooltipText.empty()) {
            ImGui::TextUnformatted(uiState_.toolTooltipText.c_str());
        }
        else {
            ImGui::Text("Selected: %d", uiState_.selectedAtomCount);
        }
        ImGui::EndTooltip();
    }
    ImGui::PopFont();

    ImGui::PushFont(fontManager.dialog);
    fileDialog.draw(styleManager.getScale());
    debugPanel.draw(styleManager.getScale(), window_->getSize());
    settingsPanel.draw(styleManager.getScale(), window_->getSize(), *simulation_, *renderer_, *captureController_, fileDialog);
    ioPanel.draw(styleManager.getScale(), window_->getSize(), *simulation_, fileDialog, uiState_);
    ImGui::PopFont();

    uiState_.cursorHovered = ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) ||
                             ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
    return EXIT_SUCCESS;
}

UiState& Interface::state() { return uiState_; }

const UiState& Interface::state() const { return uiState_; }
