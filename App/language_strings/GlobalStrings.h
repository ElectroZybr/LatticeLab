#pragma once

#include <string>
#include <unordered_map>
#include <functional>

enum class Language { en, ru };

class LanguagePath {
public:
    static std::string getPathByLanguage(Language lang);
};

class CachedString {
public:
    std::string& str();
    const char* c_str();
private:
  std::string original_str;
  bool cached = false;
  const char* cached_c_str;
};

class GlobalStrings {
public:
    CachedString integrator_velocity_verlet;
    CachedString integrator_kdk;
    CachedString integrator_runge_kutta_4;
    CachedString integrator_langevin;
    CachedString integrator_unknown;

    CachedString speed_color_normal_coloring;
    CachedString speed_color_gradient_coloring;
    CachedString speed_color_turbo_coloring;

    CachedString capture_preset_ultrafast;
    CachedString capture_preset_veryfast;
    CachedString capture_preset_faster;
    CachedString capture_preset_fast;
    CachedString capture_preset_medium;

    CachedString capture_pixel_format_Yuv420p;
    CachedString capture_pixel_format_Yuv444p;

    CachedString imgui_settings_panel;
    CachedString imgui_simulation;
    CachedString imgui_gravity;
    CachedString imgui_reset_gravity;
    CachedString imgui_gravity_x;
    CachedString imgui_gravity_y;
    CachedString imgui_gravity_z;
    CachedString imgui_integrator;
    CachedString imgui_warning_not_implemented_used_as_velocity_verlet;
    CachedString imgui_speed_of_light;
    CachedString imgui_speed_of_light_unlimited;
    CachedString imgui_accel_damping;
    CachedString imgui_time_step;
    CachedString imgui_bond_formation;
    CachedString imgui_lj;
    CachedString imgui_coulomb;
    CachedString imgui_render;
    CachedString imgui_grid;
    CachedString imgui_connections;
    CachedString imgui_color_scheme;
    CachedString imgui_speed_color_mode;
    CachedString imgui_max_gradien_velocity;
    CachedString imgui_speed_gradient_max_slider;
    CachedString imgui_auto_speed_gradien;
    CachedString imgui_neighbour_list;
    CachedString imgui_cell_size;
    CachedString imgui_cutoff_nl;
    CachedString imgui_skin_nl;
    CachedString imgui_write;
    CachedString imgui_video_saving_folder;
    CachedString imgui_capture_dir;
    CachedString imgui_capture_dir_browse;
    CachedString imgui_capture_settings_table;
    CachedString imgui_fps_capture;
    CachedString imgui_crf_capture;
    CachedString imgui_preset_capture;
    CachedString imgui_color_capture;
    CachedString imgui_reset_settings;
    CachedString imgui_exit_button;

    CachedString version_text_pre;
    CachedString version_text_after;
};

using Setter = std::function<void(GlobalStrings&, std::string)>;

// cool macro """hack"""
#define STRING_SETTER(member) \
    {#member, [](GlobalStrings& s, const std::string& v) { s.member.str() = v; }}

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
