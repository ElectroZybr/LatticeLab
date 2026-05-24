#pragma once
#include <string>

enum class Language { en, ru };

class LanguagePath {
public:
    static std::string getPathByLanguage(Language lang);
};

class GlobalStrings {
public:
    const std::string integrator_velocity_verlet;
    const std::string integrator_kdk;
    const std::string integrator_runge_kutta_4;
    const std::string integrator_langevin;
    const std::string integrator_unknown;

    const std::string speed_color_normal_coloring;
    const std::string speed_color_gradient_coloring;
    const std::string speed_color_turbo_coloring;

    const std::string capture_preset_ultrafast;
    const std::string capture_preset_veryfast;
    const std::string capture_preset_faster;
    const std::string capture_preset_fast;
    const std::string capture_preset_medium;

    const std::string capture_pixel_format_Yuv420p;
    const std::string capture_pixel_format_Yuv444p;

    const std::string imgui_settings_panel;
    const std::string imgui_simulation;
    const std::string imgui_gravity;
    const std::string imgui_reset_gravity;
    const std::string imgui_gravity_x;
    const std::string imgui_gravity_y;
    const std::string imgui_gravity_z;
    const std::string imgui_integrator;
    const std::string imgui_warning_not_implemented_used_as_velocity_verlet;
    const std::string imgui_speed_of_light;
    const std::string imgui_speed_of_light_unlimited;
    const std::string imgui_accel_damping;
    const std::string imgui_time_step;
    const std::string imgui_bond_formation;
    const std::string imgui_lj;
    const std::string imgui_coulomb;
    const std::string imgui_render;
    const std::string imgui_grid;
    const std::string imgui_connections;
    const std::string imgui_color_scheme;
    const std::string imgui_speed_color_mode;
    const std::string imgui_max_gradien_velocity;
    const std::string imgui_speed_gradient_max_slider;
    const std::string imgui_auto_speed_gradien;
    const std::string imgui_neighbour_list;
    const std::string imgui_cell_size;
    const std::string imgui_cutoff_nl;
    const std::string imgui_skin_nl;
    const std::string imgui_write;
    const std::string imgui_video_saving_folder;
    const std::string imgui_capture_dir;
    const std::string imgui_capture_dir_browse;
    const std::string imgui_capture_settings_table;
    const std::string imgui_fps_capture;
    const std::string imgui_crf_capture;
    const std::string imgui_preset_capture;
    const std::string imgui_color_capture;
    const std::string imgui_reset_settings;
    const std::string imgui_exit_button;

    const std::string version_text_pre;
    const std::string version_text_after;
};

class GlobalStringReader {
public:
    static GlobalStrings createGlobalStringByPath(std::string path);
    static GlobalStrings createGlobalStringByLanguage(Language lang);
};

static GlobalStrings global_strings{0};
