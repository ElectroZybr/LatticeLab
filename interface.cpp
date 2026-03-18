#include <SFML/Graphics.hpp>
#include "ImGuiFileDialog.h"

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
    static int decode[] = { 1, -1, -1, -1, -1, -1, -1,  2, 
                            3,  4,  5,  6,  7,  8,  9, 10,
                           11, 12, 13, 14, 15, 16, 17, 18};
    if (selectedAtom != -1)
        return decode[selectedAtom];
    return -1;
}

int Interface::Update() {
    ImGui::SFML::Update(*window, clock.restart());

    ImGui::PushFont(Rubik_VariableFont_wght);
    toolsPanel.draw(styleManager.getScale(), *window, debugPanel, fileDialog);
    ImGui::PopFont();

    const float top_panel_width = 387.0f * styleManager.getScale();
    const float top_panel_height = 142.0f * styleManager.getScale();
    const float top_panel_x = window->getSize().x * 0.5f - top_panel_width * 0.5f;
    const float tab_width = 180.0f * styleManager.getScale();
    const float tab_height = 8.0f * styleManager.getScale();
    const float tab_x = window->getSize().x * 0.5f - tab_width * 0.5f;

    static float top_panel_anim = 0.0f; // 0 - hidden, 1 - shown

    ImVec2 mouse = ImGui::GetMousePos();
    sf::Vector2i mouse_local = sf::Mouse::getPosition(*window);
    bool mouse_in_window = mouse_local.x >= 0 && mouse_local.y >= 0 &&
                           mouse_local.x < static_cast<int>(window->getSize().x) &&
                           mouse_local.y < static_cast<int>(window->getSize().y);

    bool over_tab = mouse_in_window &&
                    mouse.x >= tab_x && mouse.x <= tab_x + tab_width &&
                    mouse.y >= 0.0f && mouse.y <= tab_height;

    float hidden_y = -top_panel_height;
    float current_y = hidden_y + top_panel_anim * top_panel_height;
    bool over_panel = mouse_in_window &&
                      mouse.x >= top_panel_x && mouse.x <= top_panel_x + top_panel_width &&
                      mouse.y >= current_y && mouse.y <= current_y + top_panel_height;

    float target = (over_tab || over_panel) ? 1.0f : 0.0f;
    float step = ImGui::GetIO().DeltaTime * 12.0f;
    if (step > 1.0f) step = 1.0f;
    top_panel_anim += (target - top_panel_anim) * step;
    current_y = hidden_y + top_panel_anim * top_panel_height;

    ImGui::SetNextWindowPos(ImVec2(tab_x, 0));
    ImGui::SetNextWindowSize(ImVec2(tab_width, tab_height));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Top panel tab", nullptr,
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar
    );
    ImGui::End();
    ImGui::PopStyleVar(2);

    ImGui::SetNextWindowPos(ImVec2(top_panel_x, current_y));
    ImGui::SetNextWindowSize(ImVec2(top_panel_width, top_panel_height));
    ImGui::Begin("Top panel", nullptr, 
        ImGuiWindowFlags_NoMove |           // Запретить перемещение
        ImGuiWindowFlags_NoResize |         // Запретить изменение размера
        ImGuiWindowFlags_NoCollapse |       // Убрать кнопку сворачивания
        ImGuiWindowFlags_NoTitleBar |       // Скрыть заголовок
        ImGuiWindowFlags_NoScrollbar
    );

    static const char* keys[] = {"H",  " ",  " ",  " ",  " ", " ", " ",  "He", 
                                 "Li", "Be", "B",  "C",  "N", "O", "F",  "Ne",
                                 "Na", "Mg", "Al", "Si", "P", "S", "Cl", "Ar"};

    
    bool flag = false;

    ImGui::PushFont(Rubik_VariableFont_wght);
    for (int i = 0; i < IM_ARRAYSIZE(keys); i++) {
        // Меняем стиль для выбранной кнопки
        if (i == selectedAtom) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.06, 0.53, 0.98, 1.00));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.06, 0.53, 0.98, 1.00));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.06, 0.53, 0.98, 1.00));
        }
        
        if (ImGui::Button(keys[i], ImVec2(40*styleManager.getScale(), 40*styleManager.getScale()))) {
            flag = true;
        }
        
        if (i == selectedAtom) {
            ImGui::PopStyleColor(3);
        }
        
        if (flag) {
            if (selectedAtom != i)
                selectedAtom = i;
            else 
                selectedAtom = -1;
            flag = false;
        }
        

        // Располагаем кнопки в ряд с отступами
        if ((i + 1) % 8 != 0) ImGui::SameLine(0.0f, 7.5f*styleManager.getScale());
    }

    ImGui::PopFont();
    ImGui::End();


    
    ImGui::SetNextWindowPos(ImVec2(window->getSize().x - (122*styleManager.getScale()), 0));
    ImGui::SetNextWindowSize(ImVec2(122*styleManager.getScale(), 111*styleManager.getScale()));

    ImGui::Begin("Poops", nullptr, 
        ImGuiWindowFlags_NoMove |           // Запретить перемещение
        ImGuiWindowFlags_NoResize |         // Запретить изменение размера
        ImGuiWindowFlags_NoCollapse |       // Убрать кнопку сворачивания
        ImGuiWindowFlags_NoTitleBar |       // Скрыть заголовок
        ImGuiWindowFlags_NoScrollbar
    );
    ImGui::PushFont(Rubik_VariableFont_wght);

    static int fast = 0;    // 0 - обычная скорость, 1 - x2, 2 - x3
    if (fast == 0) {
        if (ImGui::Button(ICON_FA_FORWARD, ImVec2(50*styleManager.getScale(), 50*styleManager.getScale()))) {
            fast++;
        }
    } else if (fast == 1) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.06, 0.53, 0.98, 1.00));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.06, 0.53, 0.98, 1.00));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.06, 0.53, 0.98, 1.00));
        if (ImGui::Button(ICON_FA_FORWARD, ImVec2(50*styleManager.getScale(), 50*styleManager.getScale()))) {
            fast++;
        }
        ImGui::PopStyleColor(3);
    } else if (fast == 2) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.06, 0.53, 0.98, 1.00));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.06, 0.53, 0.98, 1.00));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.06, 0.53, 0.98, 1.00));
        if (ImGui::Button(ICON_FA_FAST_FORWARD, ImVec2(50*styleManager.getScale(), 50*styleManager.getScale()))) {
            fast = 0;
        }
        ImGui::PopStyleColor(3);
    }
    ImGui::SameLine();

    if (pause) {
        if (ImGui::Button(ICON_FA_PLAY, ImVec2(50*styleManager.getScale(), 50*styleManager.getScale()))) {
            pause = false;
        }
    } else {
        if (ImGui::Button(ICON_FA_PAUSE, ImVec2(50*styleManager.getScale(), 50*styleManager.getScale()))) {
            pause = true;
        }
    }

    ImGui::PushItemWidth(106*styleManager.getScale());
    ImGui::SliderFloat("##Speed", &simulationSpeed, 0.1, 50, "%.1f", ImGuiSliderFlags_Logarithmic);
    ImGui::PopItemWidth();

    // if (ImGui::IsItemHovered()) {
    //     ImGui::SetTooltip("podscazka");
    // }

    ImGui::PopFont();
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(window->getSize().x - (150*styleManager.getScale()), window->getSize().y - (50*styleManager.getScale())));
    ImGui::SetNextWindowSize(ImVec2(window->getSize().x, window->getSize().y));
    ImGui::Begin("Stats", nullptr, 
        ImGuiWindowFlags_NoMove |           // Запретить перемещение
        ImGuiWindowFlags_NoResize |         // Запретить изменение размера
        ImGuiWindowFlags_NoCollapse |       // Убрать кнопку сворачивания
        ImGuiWindowFlags_NoTitleBar |       // Скрыть заголовок
        ImGuiWindowFlags_NoScrollbar
    );
    ImGui::PushFont(Rubik_VariableFont_wght);
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::PopFont();
    ImGui::End();

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
