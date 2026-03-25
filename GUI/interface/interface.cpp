#include <SFML/Graphics.hpp>

#include "interface.h"
#include "imgui_impl_opengl3.h"

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
Simulation* Interface::simulation = nullptr;
std::unique_ptr<IRenderer>* Interface::renderer = nullptr;

sf::Clock Interface::clock;
int Interface::selectedAtom = 0;
bool Interface::pause;
bool Interface::cursorHovered = false;
float Interface::simulationSpeed = 100.f;
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
SettingsPanel Interface::settingsPanel;

int Interface::init(sf::RenderWindow& w, Simulation& s, std::unique_ptr<IRenderer>& r) {
    window = &w;
    simulation = &s;
    renderer = &r;

    if (!ImGui::SFML::Init(*window, false)) return EXIT_FAILURE;

    styleManager.applyCustomStyle();

    if (!fontManager.load(styleManager.getScale())) return EXIT_FAILURE;
    if (!ImGui_ImplOpenGL3_Init("#version 150")) return EXIT_FAILURE;
    if (!ImGui_ImplOpenGL3_CreateFontsTexture()) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

void Interface::shutdown() {
    ImGui_ImplOpenGL3_DestroyFontsTexture();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::SFML::Shutdown();
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
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::SFML::Update(*window, clock.restart());

    ImGui::PushFont(fontManager.main);
        toolsPanel.draw(styleManager.getScale(), *window, debugPanel, fileDialog, settingsPanel);
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
        settingsPanel.draw(styleManager.getScale(), window->getSize(), *simulation, *renderer);
    ImGui::PopFont();

    // Проверка на вхождение курсора в область
    cursorHovered = ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
    return EXIT_SUCCESS;
}
