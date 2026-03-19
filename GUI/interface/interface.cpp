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
sf::Clock Interface::clock;
int Interface::selectedAtom = -1;
bool Interface::pause;
bool Interface::cursorHovered = false;
float Interface::simulationSpeed = 1;
double Interface::averageEnergy = 0.0;
int Interface::countSelectedAtom = 0;
bool Interface::drawToolTrip = false;
int Interface::sim_step = 0;

FontManager Interface::fontManager;

DebugPanel Interface::debugPanel;
FileDialogManager Interface::fileDialog;
StyleManager Interface::styleManager;
ToolsPanel Interface::toolsPanel;
SideToolsPanel Interface::sideToolsPanel;
SimControlPanel Interface::simControlPanel;
PeriodicPanel Interface::periodicPanel;
StatsPanel Interface::statsPanel;

int Interface::init(sf::RenderWindow& w) {
    window = &w;

    if (!ImGui::SFML::Init(*window)) return EXIT_FAILURE;

    styleManager.applyCustomStyle();

    if (!fontManager.load(styleManager.getScale())) return EXIT_FAILURE;
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
        if (fontManager.load(styleManager.getScale())) {
            if (!ImGui::SFML::UpdateFontTexture()) {
                // Keep current font pointers if texture update failed.
            }
        }
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

    ImGui::PushFont(fontManager.main);
        toolsPanel.draw(styleManager.getScale(), *window, debugPanel, fileDialog);
        periodicPanel.draw(styleManager.getScale(), window->getSize(), selectedAtom);
        simControlPanel.draw(styleManager.getScale(), window->getSize(), pause, simulationSpeed);
        sideToolsPanel.draw(styleManager.getScale(), window->getSize(), fontManager.icons, fontManager.dialog);
        statsPanel.draw(styleManager.getScale(), window->getSize());
        if (drawToolTrip) {
            ImVec2 mouse = ImGui::GetMousePos();
            ImGui::SetNextWindowPos(ImVec2(mouse.x + 3 * styleManager.getScale(), mouse.y + 3 * styleManager.getScale()));
    
            ImGui::BeginTooltip();
            ImGui::Text("Selected: %d", countSelectedAtom);
            ImGui::EndTooltip();
        }
    ImGui::PopFont();

    ImGui::PushFont(fontManager.dialog);
        fileDialog.draw(styleManager.getScale());
        debugPanel.draw(styleManager.getScale(), window->getSize());
    ImGui::PopFont();

    // Проверка на вхождение курсора в область
    cursorHovered = ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
    return EXIT_SUCCESS;
}
