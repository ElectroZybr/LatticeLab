#include "App/language_strings/GlobalStrings.h"

#include <unordered_map>
#include <functional>

// cool macro """hack"""
#define STRING_SETTER(member) \
    {#member, [](GlobalStrings& s, const std::string& v) { s.member = v; }}

using Setter = std::function<void(GlobalStrings&, const std::string&)>;
std::unordered_map<std::string, Setter> setters = {
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

// i think there's a better way
std::string LanguagePath::getPathByLanguage(Language lang) {
  switch (lang) {
  case Language::en:
    return "./assets/translations/en.lang";
    break;
    
  case Language::ru:
    return "./assets/translations/ru.lang";
    break;
  }
}

// File's structure:
// string="value",
// string2= "value2",
// string3 = "value3",
// string4 = "value with spaces",
// string5 =
//      "lots of spaces that are ignored",
// string6 = "string with some \"parentesis\" inside",
GlobalStrings GlobalStringReader::createGlobalStringByPath(std::string path) {
  
}

GlobalStrings GlobalStringReader::createGlobalStringByLanguage(Language lang) {
  return GlobalStringReader::createGlobalStringByPath
	   (LanguagePath::getPathByLanguage(lang));
}
