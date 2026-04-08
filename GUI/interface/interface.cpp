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

namespace {
    void drawScenePreviewFrame(const sf::RenderWindow& window, float uiScale) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

        const float frameAspect = 1.282f;
        const float horizontalMargin = 36.0f * uiScale;
        const float verticalMargin = 88.0f * uiScale;
        const float maxWidth = std::max(120.0f, displaySize.x - horizontalMargin * 2.0f);
        const float maxHeight = std::max(120.0f, displaySize.y - verticalMargin * 2.0f);

        float frameWidth = maxWidth * 0.7f;
        float frameHeight = frameWidth / frameAspect;
        if (frameHeight > maxHeight) {
            frameHeight = maxHeight * 0.72f;
            frameWidth = frameHeight * frameAspect;
        }

        const ImVec2 frameMin((displaySize.x - frameWidth) * 0.5f, (displaySize.y - frameHeight) * 0.5f);
        const ImVec2 frameMax(frameMin.x + frameWidth, frameMin.y + frameHeight);
        const ImU32 shadeColor = ImGui::GetColorU32(ImVec4(0.02f, 0.03f, 0.05f, 0.42f));
        const ImU32 frameColor = ImGui::GetColorU32(ImVec4(0.80f, 0.88f, 1.00f, 0.95f));
        const ImU32 hintBgColor = ImGui::GetColorU32(ImVec4(0.05f, 0.07f, 0.10f, 0.78f));

        drawList->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(displaySize.x, frameMin.y), shadeColor);
        drawList->AddRectFilled(ImVec2(0.0f, frameMin.y), ImVec2(frameMin.x, frameMax.y), shadeColor);
        drawList->AddRectFilled(ImVec2(frameMax.x, frameMin.y), ImVec2(displaySize.x, frameMax.y), shadeColor);
        drawList->AddRectFilled(ImVec2(0.0f, frameMax.y), ImVec2(displaySize.x, displaySize.y), shadeColor);

        drawList->AddRect(frameMin, frameMax, frameColor, 10.0f, 0, 1.5f);
    }
}

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

    uiState_.scenePreviewMode = fileDialog.isSaveDialogOpen();
    if (uiState_.scenePreviewMode) {
        drawScenePreviewFrame(*window_, styleManager.getScale());
    }

    uiState_.cursorHovered = ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) ||
                             ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
    return EXIT_SUCCESS;
}

UiState& Interface::state() { return uiState_; }

const UiState& Interface::state() const { return uiState_; }
