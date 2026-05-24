#pragma once
#include <string>

enum class Language { en, ru };

class LanguagePath {
public:
    static std::string getPathByLanguage(Language lang);
};

class GlobalStrings {
public:
    std::string integrator_velocity_verlet;
    std::string integrator_kdk;
    std::string integrator_runge_kutta_4;
    std::string integrator_langevin;
    std::string integrator_unknown;

    std::string speed_color_normal_coloring;
    std::string speed_color_gradient_coloring;
    std::string speed_color_turbo_coloring;

    std::string capture_preset_ultrafast;
    std::string capture_preset_veryfast;
    std::string capture_preset_faster;
    std::string capture_preset_fast;
    std::string capture_preset_medium;

    std::string capture_pixel_format_Yuv420p;
    std::string capture_pixel_format_Yuv444p;

    std::string imgui_settings_panel;
    std::string imgui_simulation;
    std::string imgui_gravity;
    std::string imgui_reset_gravity;
    std::string imgui_gravity_x;
    std::string imgui_gravity_y;
    std::string imgui_gravity_z;
    std::string imgui_integrator;
    std::string imgui_warning_not_implemented_used_as_velocity_verlet;
    std::string imgui_speed_of_light;
    std::string imgui_speed_of_light_unlimited;
    std::string imgui_accel_damping;
    std::string imgui_time_step;
    std::string imgui_bond_formation;
    std::string imgui_lj;
    std::string imgui_coulomb;
    std::string imgui_render;
    std::string imgui_grid;
    std::string imgui_connections;
    std::string imgui_color_scheme;
    std::string imgui_speed_color_mode;
    std::string imgui_max_gradien_velocity;
    std::string imgui_speed_gradient_max_slider;
    std::string imgui_auto_speed_gradien;
    std::string imgui_neighbour_list;
    std::string imgui_cell_size;
    std::string imgui_cutoff_nl;
    std::string imgui_skin_nl;
    std::string imgui_write;
    std::string imgui_video_saving_folder;
    std::string imgui_capture_dir;
    std::string imgui_capture_dir_browse;
    std::string imgui_capture_settings_table;
    std::string imgui_fps_capture;
    std::string imgui_crf_capture;
    std::string imgui_preset_capture;
    std::string imgui_color_capture;
    std::string imgui_reset_settings;
    std::string imgui_exit_button;

    std::string version_text_pre;
    std::string version_text_after;
};

class GlobalStringReader {
public:
    static GlobalStrings createGlobalStringByPath(std::string path);
    static GlobalStrings createGlobalStringByLanguage(Language lang);
};

static GlobalStrings global_strings{0};
