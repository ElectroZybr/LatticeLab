#include <SFML/Graphics.hpp>

#include "interface.h"

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

sf::RenderWindow* Interface::window = nullptr;
ImGuiStyle* Interface::style = nullptr;
ImGuiStyle Interface::baseStyle;
ImFont* Interface::Rubik_VariableFont_wght = nullptr;
ImFont* Interface::Font_Awesome = nullptr;
ImFont* Interface::DialogFont = nullptr;
sf::Clock Interface::clock;
int Interface::selectedAtom = -1;
bool Interface::pause;
bool Interface::cursorHovered = false;
float Interface::simulationSpeed = 1;
double Interface::averageEnergy = 0.0;
int Interface::countSelectedAtom = 0;
bool Interface::drawToolTrip = false;
int Interface::sim_step = 0;
DebugPanel Interface::debugPanel;
FileDialogManager Interface::fileDialog;
StyleManager Interface::styleManager;
ToolsPanel Interface::toolsPanel;
SimControlPanel Interface::simControlPanel;
PeriodicPanel Interface::periodicPanel;
StatsPanel Interface::statsPanel;

int Interface::init(sf::RenderWindow& w) {
    window = &w;

    if (!ImGui::SFML::Init(*window)) return EXIT_FAILURE;

    styleManager.applyCustomStyle();

    // Загружаем шрифты
    Interface::Rubik_VariableFont_wght = ImGui::GetIO().Fonts->AddFontFromFileTTF("Engine/gui/fonts/Rubik-VariableFont_wght.ttf", 50.0f);

    // Загружаем иконки
    ImFontConfig config;
    config.MergeMode = true; // Важно!
    config.GlyphMinAdvanceX = 40.0f;
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    Interface::Font_Awesome = ImGui::GetIO().Fonts->AddFontFromFileTTF("Engine/gui/fonts/Font Awesome 5 Free-Solid-900.otf", 40.0f, &config, icon_ranges);

    Interface::DialogFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(
        "Engine/gui/fonts/Rubik-VariableFont_wght.ttf", 20.0f
    );

    Interface::debugPanel.loadFont("Engine/gui/fonts/Rubik-VariableFont_wght.ttf", 20.0f);
    if (!ImGui::SFML::UpdateFontTexture()) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

void Interface::CheckEvent(const sf::Event& event) {
    if (const auto* e = event.getIf<sf::Event::KeyPressed>()) {
        if (e->code == sf::Keyboard::Key::P) {
            debugPanel.toggle();
        }
        else if (e->code == sf::Keyboard::Key::Space) {
            pause = !pause;
        }
    }
    else if (const auto* e = event.getIf<sf::Event::Resized>()) {
        styleManager.onResize(e->size);
    }
}

float Interface::getSimulationSpeed() {
    return simulationSpeed;
}

void Interface::setAverageEnergy(double energy) {
    averageEnergy = energy;
}

void Interface::setSimStep(int step) {
    sim_step = step;
}

bool Interface::getPause() {
    return pause;
}

int Interface::getSelectedAtom() {
    return PeriodicPanel::decodeAtom(selectedAtom);
}

int Interface::Update() {
    ImGui::SFML::Update(*window, clock.restart());

    ImGui::PushFont(Rubik_VariableFont_wght);
        toolsPanel.draw(styleManager.getScale(), *window, debugPanel, fileDialog);
        periodicPanel.draw(styleManager.getScale(), window->getSize(), selectedAtom);
        simControlPanel.draw(styleManager.getScale(), window->getSize(), pause, simulationSpeed);
        statsPanel.draw(styleManager.getScale(), window->getSize());
    ImGui::PopFont();
    
    if (drawToolTrip) {
        ImVec2 mouse = ImGui::GetMousePos();
        ImGui::SetNextWindowPos(ImVec2(mouse.x + 3 * styleManager.getScale(), mouse.y + 3 * styleManager.getScale()));

        ImGui::BeginTooltip();
        if (Rubik_VariableFont_wght) {
            ImGui::PushFont(Rubik_VariableFont_wght);
        }
        ImGui::Text("Selected: %d", countSelectedAtom);
        if (Rubik_VariableFont_wght) {
            ImGui::PopFont();
        }
        ImGui::EndTooltip();
    }

    ImVec2 dlgSize(400 * styleManager.getScale(), 300 * styleManager.getScale());

    ImGui::PushFont(DialogFont);
    fileDialog.draw(styleManager.getScale());
    ImGui::PopFont();

    debugPanel.draw(styleManager.getScale(), window->getSize());

    // Проверка на вхождение курсора в область
    cursorHovered = ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
    return EXIT_SUCCESS;
}
