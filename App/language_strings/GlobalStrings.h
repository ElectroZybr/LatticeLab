#pragma once

#include <string>
#include <unordered_map>
#include <functional>

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

// cool macro """hack"""
#define STRING_SETTER(member) \
    {#member, [](GlobalStrings& s, const std::string& v) { s.member = v; }}

using Setter = std::function<void(GlobalStrings&, std::string)>;

// we may consider moving this to a source file if compile
// time becomes an issue
inline std::unordered_map<std::string, Setter> global_string_setters = {
    STRING_SETTER(integrator_velocity_verlet),
    STRING_SETTER(integrator_kdk),
    STRING_SETTER(integrator_runge_kutta_4),
    STRING_SETTER(integrator_langevin),
    STRING_SETTER(integrator_unknown),

    STRING_SETTER(speed_color_normal_coloring),
    STRING_SETTER(speed_color_gradient_coloring),
    STRING_SETTER(speed_color_turbo_coloring),

    STRING_SETTER(capture_preset_ultrafast),
    STRING_SETTER(capture_preset_veryfast),

    STRING_SETTER(capture_preset_faster),
    STRING_SETTER(capture_preset_fast),
    STRING_SETTER(capture_preset_medium),

    STRING_SETTER(capture_pixel_format_Yuv420p),
    STRING_SETTER(capture_pixel_format_Yuv444p),

    STRING_SETTER(imgui_settings_panel),
    STRING_SETTER(imgui_simulation),
    STRING_SETTER(imgui_gravity),
    STRING_SETTER(imgui_reset_gravity),
    STRING_SETTER(imgui_gravity_x),
    STRING_SETTER(imgui_gravity_y),
    STRING_SETTER(imgui_gravity_z),
    STRING_SETTER(imgui_integrator),
    STRING_SETTER(imgui_warning_not_implemented_used_as_velocity_verlet),
    STRING_SETTER(imgui_speed_of_light),
    STRING_SETTER(imgui_speed_of_light_unlimited),
    STRING_SETTER(imgui_accel_damping),
    STRING_SETTER(imgui_time_step),
    STRING_SETTER(imgui_bond_formation),
    STRING_SETTER(imgui_lj),
    STRING_SETTER(imgui_coulomb),
    STRING_SETTER(imgui_render),
    STRING_SETTER(imgui_grid),

    STRING_SETTER(imgui_connections),
    STRING_SETTER(imgui_color_scheme),
    STRING_SETTER(imgui_speed_color_mode),
    STRING_SETTER(imgui_max_gradien_velocity),
    STRING_SETTER(imgui_speed_gradient_max_slider),
    STRING_SETTER(imgui_auto_speed_gradien),
    STRING_SETTER(imgui_neighbour_list),
    STRING_SETTER(imgui_cell_size),
    STRING_SETTER(imgui_cutoff_nl),
    STRING_SETTER(imgui_skin_nl),
    STRING_SETTER(imgui_write),
    STRING_SETTER(imgui_video_saving_folder),
    STRING_SETTER(imgui_capture_dir),
    STRING_SETTER(imgui_capture_dir_browse),
    STRING_SETTER(imgui_capture_settings_table),
    STRING_SETTER(imgui_fps_capture),
    STRING_SETTER(imgui_crf_capture),
    STRING_SETTER(imgui_preset_capture),
    STRING_SETTER(imgui_color_capture),
    STRING_SETTER(imgui_reset_settings),
    STRING_SETTER(imgui_exit_button),

    STRING_SETTER(version_text_pre),
    STRING_SETTER(version_text_after),
};

inline GlobalStrings* global_strings = new GlobalStrings;
