#define _CRT_SECURE_NO_WARNINGS
#include "config.h"
#include <shlobj.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "../../output/output.h"

#pragma comment(lib, "shell32.lib")

namespace config
{
    struct item_t
    {
        std::string name;
        void* ptr;
        std::string type;
        int size = 1;
    };

    std::vector<item_t> items;

    void add_item(const std::string& name, void* ptr, const std::string& type, int size = 1)
    {
        items.push_back({ name, ptr, type, size });
    }

    void init()
    {
        static bool initialized = false;
        if (initialized) return;
        add_item("teamcheck", &globals::settings::teamcheck, "bool");
        add_item("clientcheck", &globals::settings::clientcheck, "bool");
        add_item("streamproof", &globals::settings::streamproof, "bool");
        add_item("dpi_scaling", &globals::settings::dpi_scaling, "float");
        add_item("performance_def", &globals::settings::performance_def, "int");
        add_item("low_latency", &globals::settings::low_latency, "bool");
        add_item("rendering_quality", &globals::settings::rendering_quality, "int");
        add_item("show_fps", &globals::settings::show_fps, "bool");
        add_item("watermark", &globals::settings::watermark, "bool");
        add_item("aimbot_enabled", &globals::aimbot::enabled, "bool");
        add_item("aimbot_method", &globals::aimbot::method, "int");
        add_item("aimbot_fov", &globals::aimbot::fov, "float");
        add_item("aimbot_radius", &globals::aimbot::radius, "int");
        add_item("aimbot_smoothing", &globals::aimbot::smoothing, "bool");
        add_item("aimbot_smoothing_x", &globals::aimbot::smoothing_values[0], "float");
        add_item("aimbot_smoothing_y", &globals::aimbot::smoothing_values[1], "float");
        add_item("aimbot_keybind", &globals::aimbot::keybind, "int");
        add_item("aimbot_keybind_mode", &globals::aimbot::keybind_mode, "int");
        add_item("aimbot_wallcheck", &globals::aimbot::wallcheck, "bool");
        add_item("aimbot_auto_wall", &globals::aimbot::auto_wall, "bool");
        for (int i = 0; i < 8; i++) {
            add_item("aimbot_check_" + std::to_string(i), &globals::aimbot::aiming_checks[i], "int");
        }

        add_item("prediction_enabled", &globals::aimbot::prediction::enabled, "bool");
        add_item("prediction_velocity", &globals::aimbot::prediction::velocity_prediction, "bool");
        add_item("prediction_multiplier", &globals::aimbot::prediction::multiplier_values[0], "float");
        add_item("prediction_strength", &globals::aimbot::prediction::multiplier_values[1], "float");
        add_item("prediction_x_multiplier", &globals::aimbot::prediction::x_multiplier, "float");
        add_item("prediction_y_multiplier", &globals::aimbot::prediction::y_multiplier, "float");
        add_item("prediction_bullet_speed_enabled", &globals::aimbot::prediction::bullet_speed, "bool");
        add_item("prediction_bullet_speed", &globals::aimbot::prediction::bullet_speed_value, "float");
        add_item("prediction_bullet_gravity_enabled", &globals::aimbot::prediction::bullet_gravity, "bool");
        add_item("prediction_bullet_gravity", &globals::aimbot::prediction::bullet_gravity_value, "float");

        add_item("triggerbot_enabled", &globals::aimbot::trigger::enabled, "bool");
        add_item("triggerbot_delay", &globals::aimbot::trigger::delay, "float");
        add_item("triggerbot_interval", &globals::aimbot::trigger::interval, "float");
        add_item("triggerbot_hit_chance", &globals::aimbot::trigger::hit_chance, "float");

        add_item("target_strafe_enabled", &globals::aimbot::target_strafe::enabled, "bool");
        add_item("target_strafe_speed", &globals::aimbot::target_strafe::speed, "float");
        add_item("target_strafe_radius", &globals::aimbot::target_strafe::radius, "float");
        add_item("target_strafe_height", &globals::aimbot::target_strafe::height, "float");

        add_item("ideal_peak_enabled", &globals::misc::ideal_peak, "bool");
        add_item("ideal_peak_key", &globals::misc::ideal_peak_key, "int");
        add_item("ideal_peak_mode", &globals::misc::ideal_peak_mode, "int");

        add_item("recoil_reduction", &globals::misc::recoil_reduction, "bool");
        add_item("recoil_reduction_value", &globals::misc::recoil_reduction_value, "float");

        add_item("speed_hack", &globals::misc::speed_hack, "bool");
        add_item("speed_key", &globals::misc::speed_key, "int");
        add_item("speed_mode", &globals::misc::speed_mode, "int");
        add_item("speed_method", &globals::misc::speed_method, "int");
        add_item("speed_value", &globals::misc::speed_value, "float");

        add_item("fly_hack", &globals::misc::fly_hack, "bool");
        add_item("fly_key", &globals::misc::fly_key, "int");
        add_item("fly_mode", &globals::misc::fly_mode, "int");
        add_item("fly_method", &globals::misc::fly_method, "int");
        add_item("fly_speed", &globals::misc::fly_speed, "float");

        add_item("free_camera", &globals::misc::free_camera, "bool");
        add_item("free_camera_key", &globals::misc::free_camera_key, "int");
        add_item("free_camera_mode", &globals::misc::free_camera_mode, "int");
        add_item("free_camera_method", &globals::misc::free_camera_method, "int");
        add_item("free_camera_speed", &globals::misc::free_camera_speed, "float");

        add_item("long_neck", &globals::misc::long_neck, "bool");
        add_item("neck_length", &globals::misc::neck_length, "float");
        add_item("no_fall_damage", &globals::misc::no_fall_damage, "bool");
        add_item("click_tp", &globals::misc::click_tp, "bool");
        add_item("click_tp_key", &globals::misc::click_tp_key, "int");
        add_item("click_tp_mode", &globals::misc::click_tp_mode, "int");
        add_item("spiderman", &globals::misc::spiderman, "bool");
        add_item("fly_swim", &globals::misc::fly_swim, "bool");
        add_item("vehicle_fly", &globals::misc::vehicle_fly, "bool");
        add_item("vehicle_fly_speed", &globals::misc::vehicle_fly_speed, "float");
        add_item("no_send", &globals::misc::no_send, "bool");
        add_item("jump_power_enabled", &globals::misc::jump_power_enabled, "bool");
        add_item("jump_power_key", &globals::misc::jump_power_key, "int");
        add_item("jump_power_mode", &globals::misc::jump_power_mode, "int");
        add_item("jump_power_value", &globals::misc::jump_power_value, "float");


        add_item("hip_height_enabled", &globals::misc::hip_height_enabled, "bool");
        add_item("hip_height_key", &globals::misc::hip_height_key, "int");
        add_item("hip_height_mode", &globals::misc::hip_height_mode, "int");
        add_item("hip_height_value", &globals::misc::hip_height_value, "float");

        add_item("gravity_enabled", &globals::misc::gravity_enabled, "bool");
        add_item("gravity_key", &globals::misc::gravity_key, "int");
        add_item("gravity_mode", &globals::misc::gravity_mode, "int");
        add_item("gravity_value", &globals::misc::gravity_value, "float");

        add_item("sit", &globals::misc::sit, "bool");
        add_item("platform_stand", &globals::misc::platform_stand, "bool");

        add_item("jump_bug", &globals::misc::jump_bug, "bool");
        add_item("jump_bug_key", &globals::misc::jump_bug_key, "int");
        add_item("jump_bug_mode", &globals::misc::jump_bug_mode, "int");

        add_item("no_rotate", &globals::misc::no_rotate, "bool");
        add_item("no_clip", &globals::misc::no_clip, "bool");
        add_item("no_clip_key", &globals::misc::no_clip_key, "int");
        add_item("no_clip_mode", &globals::misc::no_clip_mode, "int");
        add_item("no_clip_method", &globals::misc::no_clip_method, "int");

        add_item("camera_offset_enabled", &globals::misc::camera_offset_enabled, "bool");
        add_item("camera_offset_0", &globals::misc::camera_offset[0], "float");
        add_item("camera_offset_1", &globals::misc::camera_offset[1], "float");
        add_item("camera_offset_2", &globals::misc::camera_offset[2], "float");

        add_item("camera_zoom_min", &globals::misc::camera_zoom[0], "float");
        add_item("camera_zoom_max", &globals::misc::camera_zoom[1], "float");

        add_item("fov_misc", &globals::misc::fov, "float");

        add_item("zoom_enabled", &globals::misc::zoom_enabled, "bool");
        add_item("zoom_key", &globals::misc::zoom_key, "int");
        add_item("zoom_mode", &globals::misc::zoom_mode, "int");
        add_item("zoom_value", &globals::misc::zoom_value, "float");
        add_item("angles_enabled", &globals::misc::angles_enabled, "bool");
        add_item("angles_key", &globals::misc::angles_key, "int");
        add_item("angles_mode", &globals::misc::angles_mode, "int");
        add_item("pitch_base", &globals::misc::pitch_base, "int");
        add_item("pitch_value", &globals::misc::pitch_value, "float");
        add_item("yaw_base", &globals::misc::yaw_base, "int");
        add_item("yaw_value", &globals::misc::yaw_value, "float");
        add_item("spin_speed", &globals::misc::spin_speed, "float");
        add_item("upside_down", &globals::misc::upside_down, "bool");
        add_item("yaw_jitter", &globals::misc::yaw_jitter, "bool");
        add_item("jitter_value", &globals::misc::jitter_value, "float");
        add_item("roll", &globals::misc::roll, "bool");
        add_item("anchor_on_bind", &globals::misc::anchor_on_bind, "bool");
        add_item("anchor_key", &globals::misc::anchor_key, "int");
        add_item("anchor_mode", &globals::misc::anchor_mode, "int");

        add_item("waypoint_on_bind", &globals::misc::waypoint_on_bind, "bool");
        add_item("waypoint_key", &globals::misc::waypoint_key, "int");
        add_item("waypoint_mode", &globals::misc::waypoint_mode, "int");
        add_item("waypoint_on_respawn", &globals::misc::waypoint_on_respawn, "bool");

        add_item("hit_sounds", &globals::misc::hit_sounds, "bool");
        add_item("hit_sounds_key", &globals::misc::hit_sounds_key, "int");
        add_item("hit_sounds_mode", &globals::misc::hit_sounds_mode, "int");
        add_item("hit_sound_type", &globals::misc::hit_sound_type, "int");

        add_item("death_sounds", &globals::misc::death_sounds, "bool");
        add_item("death_sounds_key", &globals::misc::death_sounds_key, "int");
        add_item("death_sounds_mode", &globals::misc::death_sounds_mode, "int");
        add_item("death_sound_type", &globals::misc::death_sound_type, "int");

        add_item("ping_spike", &globals::misc::ping_spike, "bool");
        add_item("ping_spike_key", &globals::misc::ping_spike_key, "int");
        add_item("ping_spike_mode", &globals::misc::ping_spike_mode, "int");
        add_item("ping_spike_value", &globals::misc::ping_spike_value, "float");

        add_item("desync", &globals::misc::desync, "bool");
        add_item("desync_key", &globals::misc::desync_key, "int");
        add_item("desync_mode", &globals::misc::desync_mode, "int");

        add_item("tickrate_manipulation", &globals::misc::tickrate_manipulation, "bool");
        add_item("tickrate_value", &globals::misc::tickrate_value, "float");
        add_item("vis_masterswitch", &globals::visuals::masterswitch, "bool");
        add_item("vis_show_visuals", &globals::visuals::show_visuals, "bool");
        add_item("vis_fov", &globals::visuals::fov, "float");
        add_item("vis_wallcheck", &globals::visuals::options::wallcheck, "bool");

        add_item("no_shadows", &globals::visuals::no_shadows, "bool");
        add_item("no_shadows_key", &globals::visuals::no_shadows_key, "int");
        add_item("no_shadows_mode", &globals::visuals::no_shadows_mode, "int");

        add_item("fullbright", &globals::visuals::fullbright, "bool");
        add_item("fullbright_key", &globals::visuals::fullbright_key, "int");
        add_item("fullbright_mode", &globals::visuals::fullbright_mode, "int");

        add_item("glow_strength", &globals::visuals::glow_strength, "float");

        for (int i = 0; i < 4; i++) {
            add_item("ambient_color_" + std::to_string(i), &globals::visuals::ambient_color[i], "float");
            add_item("outdoor_ambient_color_" + std::to_string(i), &globals::visuals::outdoor_ambient_color[i], "float");
            add_item("colorshift_top_" + std::to_string(i), &globals::visuals::colorshift_top[i], "float");
            add_item("colorshift_bottom_" + std::to_string(i), &globals::visuals::colorshift_bottom[i], "float");
            add_item("fog_color_" + std::to_string(i), &globals::visuals::fog_color[i], "float");
            add_item("shadow_color_" + std::to_string(i), &globals::visuals::shadow_color[i], "float");
        }

        add_item("brightness", &globals::visuals::brightness, "float");
        add_item("fog_density", &globals::visuals::fog_density, "float");
        add_item("fog_start", &globals::visuals::fog_start, "float");
        add_item("fog_end", &globals::visuals::fog_end, "float");
        add_item("exposure", &globals::visuals::exposure, "float");
        add_item("geographic_latitude", &globals::visuals::geographic_latitude, "float");

        add_item("map_debug", &globals::visuals::map_debug, "bool");
        add_item("no_fog", &globals::visuals::no_fog, "bool");
        add_item("no_fog_key", &globals::visuals::no_fog_key, "int");
        add_item("no_fog_mode", &globals::visuals::no_fog_mode, "int");

        add_item("force_lighting", &globals::visuals::force_lighting, "bool");
        add_item("celestial_bodies", &globals::visuals::celestial_bodies, "bool");

        add_item("world_gravity", &globals::visuals::world_gravity, "float");
        add_item("world_jump_power", &globals::visuals::jump_power, "float");
        add_item("world_walk_speed", &globals::visuals::walk_speed, "float");
        add_item("world_hip_height", &globals::visuals::hip_height, "float");

        add_item("auto_jump", &globals::visuals::auto_jump, "bool");
        add_item("auto_rotate", &globals::visuals::auto_rotate, "bool");

        add_item("time_of_day", &globals::visuals::time_of_day, "float");
        add_item("star_count", &globals::visuals::star_count, "int");
        add_item("sun_size", &globals::visuals::sun_size, "float");
        add_item("moon_size", &globals::visuals::moon_size, "float");

        add_item("third_person", &globals::visuals::third_person, "bool");
        add_item("vis_min_zoom", &globals::visuals::min_zoom, "float");
        add_item("vis_max_zoom", &globals::visuals::max_zoom, "float");

        add_item("fov_circle_enabled", &globals::visuals::fov_circle::enabled, "bool");
        add_item("fov_circle_team_color", &globals::visuals::fov_circle::team_color, "bool");
        add_item("fov_circle_radius", &globals::visuals::fov_circle::radius, "float");
        add_item("fov_circle_thickness", &globals::visuals::fov_circle::thickness, "float");
        add_item("fov_circle_color", &globals::visuals::fov_circle::color, "color");
        add_item("fov_circle_filled", &globals::visuals::fov_circle::filled, "bool");
        add_item("fov_circle_fill_color", &globals::visuals::fov_circle::fill_color, "color");
        add_item("fov_circle_num_sides", &globals::visuals::fov_circle::num_sides, "int");
        add_item("fov_circle_origin", &globals::visuals::fov_circle::origin, "int");
        add_item("fov_circle_rainbow", &globals::visuals::fov_circle::rainbow, "bool");
        add_item("fov_circle_rainbow_speed", &globals::visuals::fov_circle::rainbow_speed, "float");
        add_item("fov_circle_dynamic", &globals::visuals::fov_circle::dynamic, "bool");

        add_item("xhair_enabled", &globals::visuals::crosshair::enabled, "bool");
        add_item("xhair_team_color", &globals::visuals::crosshair::team_color, "bool");
        add_item("xhair_spin", &globals::visuals::crosshair::spin, "bool");
        add_item("xhair_spin_speed", &globals::visuals::crosshair::spin_speed, "float");
        add_item("xhair_pulse", &globals::visuals::crosshair::pulse, "bool");
        add_item("xhair_pulse_speed", &globals::visuals::crosshair::pulse_speed, "float");
        add_item("xhair_origin", &globals::visuals::crosshair::origin, "int");
        add_item("xhair_lerp", &globals::visuals::crosshair::lerp, "bool");
        add_item("xhair_lerp_speed", &globals::visuals::crosshair::lerp_speed, "float");
        add_item("xhair_size", &globals::visuals::crosshair::size, "float");
        add_item("xhair_thickness", &globals::visuals::crosshair::thickness, "float");
        add_item("xhair_gap", &globals::visuals::crosshair::gap, "float");
        add_item("xhair_dot", &globals::visuals::crosshair::dot, "bool");
        add_item("xhair_dot_size", &globals::visuals::crosshair::dot_size, "float");
        add_item("xhair_outline", &globals::visuals::crosshair::outline, "bool");
        add_item("xhair_outline_thickness", &globals::visuals::crosshair::outline_thickness, "float");
        add_item("xhair_t_style", &globals::visuals::crosshair::t_style, "bool");
        add_item("xhair_sniper", &globals::visuals::crosshair::sniper_crosshair, "bool");
        add_item("xhair_rainbow", &globals::visuals::crosshair::rainbow, "bool");
        add_item("xhair_rainbow_speed", &globals::visuals::crosshair::rainbow_speed, "float");
        add_item("xhair_color", &globals::visuals::crosshair::color, "color");
        add_item("xhair_dot_color", &globals::visuals::crosshair::dot_color, "color");
        add_item("xhair_outline_color", &globals::visuals::crosshair::outline_color, "color");

        auto add_player_settings = [&](const std::string& prefix, globals::visuals::player_settings_t* s) {
            add_item(prefix + "_enabled", &s->enabled, "bool");
            add_item(prefix + "_name", &s->name, "bool");
            add_item(prefix + "_name_glow", &s->name_glow, "bool");
            add_item(prefix + "_box", &s->bounding_box, "bool");
            add_item(prefix + "_box_type", &s->bounding_box_type, "int");
            add_item(prefix + "_box_style", &s->bounding_box_style, "int");
            add_item(prefix + "_box_glow", &s->box_glow, "bool");
            add_item(prefix + "_healthbar", &s->healthbar, "bool");
            add_item(prefix + "_healthbar_glow", &s->healthbar_glow, "bool");
            add_item(prefix + "_health_value", &s->health_value, "bool");
            add_item(prefix + "_arrows", &s->arrows, "bool");
            add_item(prefix + "_arrows_glow", &s->arrows_glow, "bool");
            add_item(prefix + "_arrows_size", &s->arrows_size, "float");
            add_item(prefix + "_arrows_position", &s->arrows_position, "float");
            add_item(prefix + "_arrow_type", &s->arrow_type, "int");
            add_item(prefix + "_arrows_max_dist", &s->arrows_max_dist, "float");

            for (int i = 0; i < 2; i++) {
                add_item(prefix + "_arrows_info_" + std::to_string(i), &s->arrows_info[i], "int");
            }

            add_item(prefix + "_head_dot", &s->head_dot, "bool");
            add_item(prefix + "_head_dot_glow", &s->head_dot_glow, "bool");
            add_item(prefix + "_tool", &s->tool, "bool");
            add_item(prefix + "_distance", &s->distance, "bool");
            add_item(prefix + "_distance_glow", &s->distance_glow, "bool");
            add_item(prefix + "_state_flags", &s->state_flags, "bool");
            add_item(prefix + "_rig_flags", &s->rig_flags, "bool");

            add_item(prefix + "_chams", &s->chams, "bool");
            add_item(prefix + "_chams_glow", &s->chams_glow, "bool");
            add_item(prefix + "_chams_health_based", &s->chams_health_based, "bool");
            add_item(prefix + "_chams_texture", &s->chams_texture, "int");
            add_item(prefix + "_chams_flash", &s->chams_flash, "bool");
            add_item(prefix + "_chams_flash_speed", &s->chams_flash_speed, "float");
            add_item(prefix + "_chams_fade", &s->chams_fade, "bool");
            add_item(prefix + "_chams_fade_speed", &s->chams_fade_speed, "float");

            add_item(prefix + "_mesh_chams", &s->mesh_chams, "bool");
            add_item(prefix + "_mesh_chams_glow", &s->mesh_chams_glow, "bool");

            add_item(prefix + "_skeleton", &s->skeleton, "bool");
            add_item(prefix + "_skeleton_glow", &s->skeleton_glow, "bool");

            add_item(prefix + "_sound_esp", &s->sound_esp, "bool");
            add_item(prefix + "_sound_esp_glow", &s->sound_esp_glow, "bool");
            add_item(prefix + "_sound_esp_radius", &s->sound_esp_radius, "float");
            add_item(prefix + "_sound_esp_speed", &s->sound_esp_speed, "float");

            add_item(prefix + "_footprints", &s->footprints, "bool");
            add_item(prefix + "_footprints_glow", &s->footprints_glow, "bool");
            add_item(prefix + "_footprints_radius", &s->footprints_radius, "float");
            add_item(prefix + "_footprints_speed", &s->footprints_speed, "float");

            add_item(prefix + "_visibility_colors", &s->visibility_colors, "bool");
            add_item(prefix + "_name_vis_color", &s->colors.name_vis, "color");
            add_item(prefix + "_name_invis_color", &s->colors.name_invis, "color");
            add_item(prefix + "_box_vis_color", &s->colors.box_vis, "color");
            add_item(prefix + "_box_invis_color", &s->colors.box_invis, "color");

            add_item(prefix + "_tracers", &s->tracers, "bool");
            add_item(prefix + "_tracers_thickness", &s->tracers_thickness, "float");
            add_item(prefix + "_tracers_origin", &s->tracers_origin, "int");

            add_item(prefix + "_box_fill", &s->box_fill, "bool");
            add_item(prefix + "_box_fill_gradient", &s->box_fill_gradient, "bool");
            add_item(prefix + "_box_fill_gradient_spin", &s->box_fill_gradient_spin, "bool");
            add_item(prefix + "_box_fill_gradient_speed", &s->box_fill_gradient_speed, "float");
            add_item(prefix + "_box_fill_gradient_style", &s->box_fill_gradient_style, "int");

            add_item(prefix + "_healthbar_gradient", &s->healthbar_gradient, "bool");
            add_item(prefix + "_healthbar_text", &s->healthbar_text, "bool");
            add_item(prefix + "_healthbar_lerp", &s->healthbar_lerp, "bool");

            add_item(prefix + "_box_team_color", &s->box_team_color, "bool");
            add_item(prefix + "_chams_team_color", &s->chams_team_color, "bool");
            add_item(prefix + "_arrows_team_color", &s->arrows_team_color, "bool");
            add_item(prefix + "_name_team_color", &s->name_team_color, "bool");
            add_item(prefix + "_head_dot_team_color", &s->head_dot_team_color, "bool");
            add_item(prefix + "_tool_team_color", &s->tool_team_color, "bool");
            add_item(prefix + "_distance_team_color", &s->distance_team_color, "bool");
            add_item(prefix + "_tracers_team_color", &s->tracers_team_color, "bool");
            add_item(prefix + "_skeleton_team_color", &s->skeleton_team_color, "bool");
            add_item(prefix + "_healthbar_team_color", &s->healthbar_team_color, "bool");
            add_item(prefix + "_sound_esp_team_color", &s->sound_esp_team_color, "bool");
            add_item(prefix + "_footprints_team_color", &s->footprints_team_color, "bool");
            add_item(prefix + "_mesh_chams_team_color", &s->mesh_chams_team_color, "bool");
            add_item(prefix + "_box_fill_team_color", &s->box_fill_team_color, "bool");
            add_item(prefix + "_health_value_team_color", &s->health_value_team_color, "bool");
            add_item(prefix + "_chams_flash_team_color", &s->chams_flash_team_color, "bool");
            add_item(prefix + "_clr_name", &s->colors.name, "color");
            add_item(prefix + "_clr_name_glow", &s->colors.name_glow, "color");
            add_item(prefix + "_clr_box", &s->colors.box, "color");
            add_item(prefix + "_clr_box_glow", &s->colors.box_glow, "color");
            add_item(prefix + "_clr_box_fill_0", &s->colors.box_fill[0], "color");
            add_item(prefix + "_clr_box_fill_1", &s->colors.box_fill[1], "color");
            add_item(prefix + "_clr_healthbar", &s->colors.healthbar, "color");
            add_item(prefix + "_clr_healthbar_glow", &s->colors.healthbar_glow, "color");
            add_item(prefix + "_clr_healthbar_text", &s->colors.healthbar_text, "color");
            add_item(prefix + "_clr_healthbar_gradient_0", &s->colors.healthbar_gradient[0], "color");
            add_item(prefix + "_clr_healthbar_gradient_1", &s->colors.healthbar_gradient[1], "color");
            add_item(prefix + "_clr_healthbar_gradient_2", &s->colors.healthbar_gradient[2], "color");
            add_item(prefix + "_clr_arrows", &s->colors.arrows, "color");
            add_item(prefix + "_clr_arrows_glow", &s->colors.arrows_glow, "color");
            add_item(prefix + "_clr_head_dot", &s->colors.head_dot, "color");
            add_item(prefix + "_clr_head_dot_glow", &s->colors.head_dot_glow, "color");
            add_item(prefix + "_clr_tool", &s->colors.tool, "color");
            add_item(prefix + "_clr_distance", &s->colors.distance, "color");
            add_item(prefix + "_clr_distance_glow", &s->colors.distance_glow, "color");
            add_item(prefix + "_clr_chams", &s->colors.chams, "color");
            add_item(prefix + "_clr_chams_glow", &s->colors.chams_glow, "color");
            add_item(prefix + "_clr_chams_flash", &s->colors.chams_flash, "color");
            add_item(prefix + "_clr_chams_fresnel", &s->colors.chams_fresnel, "color");
            add_item(prefix + "_clr_mesh_chams", &s->colors.mesh_chams, "color");
            add_item(prefix + "_clr_mesh_chams_glow", &s->colors.mesh_chams_glow, "color");
            add_item(prefix + "_clr_skeleton", &s->colors.skeleton, "color");
            add_item(prefix + "_clr_skeleton_glow", &s->colors.skeleton_glow, "color");
            add_item(prefix + "_clr_sound_esp", &s->colors.sound_esp, "color");
            add_item(prefix + "_clr_sound_esp_glow", &s->colors.sound_esp_glow, "color");
            add_item(prefix + "_clr_footprints", &s->colors.footprints, "color");
            add_item(prefix + "_clr_footprints_glow", &s->colors.footprints_glow, "color");
            add_item(prefix + "_clr_tracers", &s->colors.tracers, "color");
        };

        add_player_settings("enemies", &globals::visuals::enemies);
        add_player_settings("friendlies", &globals::visuals::friendlies);
        add_player_settings("local", &globals::visuals::local_player);
        add_item("client_corpse_chams", &globals::visuals::client::corpse_chams, "bool");
        add_item("client_corpse_chams_team", &globals::visuals::client::corpse_chams_team_color, "bool");
        add_item("client_corpse_names", &globals::visuals::client::corpse_names, "bool");
        add_item("client_corpse_names_team", &globals::visuals::client::corpse_names_team_color, "bool");
        add_item("client_corpse_type", &globals::visuals::client::corpse_type, "int");
        add_item("client_corpse_shift", &globals::visuals::client::corpse_shift, "bool");
        add_item("client_corpse_max_dist", &globals::visuals::client::corpse_max_dist, "float");
        add_item("client_enemy_highlight", &globals::visuals::client::enemy_highlight, "bool");
        add_item("client_enemy_highlight_team", &globals::visuals::client::enemy_highlight_team_color, "bool");
        add_item("client_friend_highlight", &globals::visuals::client::friend_highlight, "bool");
        add_item("client_friend_highlight_team", &globals::visuals::client::friend_highlight_team_color, "bool");
        add_item("client_clr_corpse_chams", &globals::visuals::client::colors::corpse_chams, "color");
        add_item("client_clr_corpse_names", &globals::visuals::client::colors::corpse_names, "color");
        add_item("client_clr_enemy_highlight", &globals::visuals::client::colors::enemy_highlight, "color");
        add_item("client_clr_friend_highlight", &globals::visuals::client::colors::friend_highlight, "color");
        add_item("opt_visible_check", &globals::visuals::options::visible_check, "bool");
        add_item("opt_forcefield_check", &globals::visuals::options::forcefield_check, "bool");
        add_item("opt_alive_check", &globals::visuals::options::alive_check, "bool");
        add_item("opt_tool_check", &globals::visuals::options::tool_check, "bool");
        add_item("opt_ragdoll_check", &globals::visuals::options::ragdoll_check, "bool");
        add_item("opt_godded_check", &globals::visuals::options::godded_check, "bool");
        add_item("opt_sort_by_status", &globals::visuals::options::sort_by_status, "bool");
        add_item("opt_max_distance", &globals::visuals::options::max_distance, "float");
        add_item("opt_name_transform", &globals::visuals::options::name_transform, "int");
        add_item("opt_max_size", &globals::visuals::options::max_size, "float");
        add_item("opt_font_type", &globals::visuals::options::font_type, "int");
        add_item("opt_global_outline_type", &globals::visuals::options::global_outline_type, "int");

        for (int i = 0; i < 9; i++) {
            add_item("opt_render_outlines_" + std::to_string(i), &globals::visuals::options::render_outlines[i], "int");
        }

        add_item("opt_outline_thickness", &globals::visuals::options::outline_thickness, "float");
        add_item("opt_chams_outline_thickness", &globals::visuals::options::chams_outline_thickness, "float");
        add_item("opt_skeleton_outline_thickness", &globals::visuals::options::skeleton_outline_thickness, "float");
        add_item("opt_text_outline_thickness", &globals::visuals::options::text_outline_thickness, "float");
        add_item("opt_use_display_name", &globals::visuals::options::use_display_name, "bool");
        add_item("opt_combined_name", &globals::visuals::options::combined_name, "bool");
        add_item("opt_box_inline", &globals::visuals::options::box_inline, "bool");
        add_item("opt_box_type", &globals::visuals::options::box_type, "int");
        add_item("opt_bounding_type", &globals::visuals::options::bounding_type, "int");
        add_item("opt_static_on_death", &globals::visuals::options::static_on_death, "bool");
        add_item("opt_static_on_void", &globals::visuals::options::static_on_void, "bool");
        add_item("opt_gradient_bars", &globals::visuals::options::gradient_bars, "bool");
        add_item("opt_bar_fold", &globals::visuals::options::bar_fold, "bool");
        add_item("opt_bar_size", &globals::visuals::options::bar_size, "float");
        add_item("opt_value_percentages", &globals::visuals::options::value_percentages, "bool");
        add_item("opt_value_follow", &globals::visuals::options::value_follow, "bool");
        add_item("opt_ignore_full", &globals::visuals::options::ignore_full, "bool");
        add_item("opt_head_type", &globals::visuals::options::head_type, "int");
        add_item("opt_head_mode", &globals::visuals::options::head_mode, "int");
        add_item("opt_head_size_limit", &globals::visuals::options::head_size_limit, "float");
        add_item("opt_head_static_size", &globals::visuals::options::head_static_size, "float");
        add_item("opt_circular_head_dot", &globals::visuals::options::circular_head_dot, "bool");
        add_item("opt_show_none", &globals::visuals::options::show_none, "bool");
        add_item("opt_bounding_box_fill", &globals::visuals::options::bounding_box_fill, "bool");
        add_item("opt_box_fill_gradient", &globals::visuals::options::box_fill_gradient, "bool");
        add_item("opt_box_fill_gradient_spin", &globals::visuals::options::box_fill_gradient_spin, "bool");
        add_item("opt_box_glow", &globals::visuals::options::box_glow, "bool");
        add_item("opt_healthbar_gradient", &globals::visuals::options::healthbar_gradient, "bool");
        add_item("opt_healthbar_glow", &globals::visuals::options::healthbar_glow, "bool");
        add_item("opt_healthbar_text", &globals::visuals::options::healthbar_text, "bool");
        add_item("opt_name_glow", &globals::visuals::options::name_glow, "bool");
        add_item("opt_distance_glow", &globals::visuals::options::distance_glow, "bool");
        add_item("opt_chams_hit_impact", &globals::visuals::options::chams_hit_impact, "bool");
        add_item("opt_chams_hit_impact_team", &globals::visuals::options::chams_hit_impact_team_color, "bool");
        add_item("opt_chams_fresnel", &globals::visuals::options::chams_fresnel, "bool");
        add_item("opt_offscreen_arrows_mode", &globals::visuals::options::offscreen_arrows_mode, "int");
        add_item("opt_offscreen_arrows_radius", &globals::visuals::options::offscreen_arrows_radius, "float");
        add_item("opt_offscreen_arrows_size", &globals::visuals::options::offscreen_arrows_size, "float");
        add_item("opt_offscreen_arrows_glow", &globals::visuals::options::offscreen_arrows_glow, "bool");
        add_item("glb_clr_box_fill_0", &globals::visuals::colors::box_fill[0], "color");
        add_item("glb_clr_box_fill_1", &globals::visuals::colors::box_fill[1], "color");
        add_item("glb_clr_healthbar_0", &globals::visuals::colors::healthbar[0], "color");
        add_item("glb_clr_healthbar_1", &globals::visuals::colors::healthbar[1], "color");
        add_item("glb_clr_healthbar_2", &globals::visuals::colors::healthbar[2], "color");
        add_item("glb_clr_healthbar_text", &globals::visuals::colors::healthbar_text, "color");
        add_item("glb_clr_chams_hit_impact", &globals::visuals::colors::chams_hit_impact, "color");
        add_item("glb_clr_chams_fresnel", &globals::visuals::colors::chams_fresnel, "color");

        add_item("viewmodel_enabled", &globals::visuals::viewmodel::enabled, "bool");
        add_item("viewmodel_fov", &globals::visuals::viewmodel::fov, "float");
        add_item("viewmodel_offset_x", &globals::visuals::viewmodel::offset[0], "float");
        add_item("viewmodel_offset_y", &globals::visuals::viewmodel::offset[1], "float");
        add_item("viewmodel_offset_z", &globals::visuals::viewmodel::offset[2], "float");

        initialized = true;
    }

    void reset()
    {
        globals::settings::teamcheck = false;
        globals::settings::clientcheck = false;
        globals::settings::streamproof = false;
        globals::settings::dpi_scaling = 1.0f;
        globals::settings::performance_def = 0;
        globals::settings::low_latency = false;
        globals::settings::rendering_quality = 10;
        globals::settings::show_fps = false;
        globals::settings::watermark = false;
        globals::aimbot::target_strafe::enabled = false;
        globals::aimbot::target_strafe::speed = 10.0f;
        globals::aimbot::target_strafe::radius = 10.0f;
        globals::aimbot::target_strafe::height = 5.0f;

        globals::misc::ideal_peak = false;
        globals::misc::ideal_peak_key = 0;
        globals::misc::ideal_peak_mode = 0;

        globals::misc::recoil_reduction = false;
        globals::misc::recoil_reduction_value = 0.0f;

        globals::misc::speed_hack = false;
        globals::misc::speed_key = 'H';
        globals::misc::speed_mode = 0;
        globals::misc::speed_method = 0;
        globals::misc::speed_value = 16.0f;
        globals::misc::fly_hack = false;
        globals::misc::fly_key = 'J';
        globals::misc::fly_mode = 0;
        globals::misc::fly_method = 0;
        globals::misc::fly_speed = 50.0f;
        globals::misc::free_camera = false;
        globals::misc::free_camera_key = 'C';
        globals::misc::free_camera_mode = 0;
        globals::misc::free_camera_method = 0;
        globals::misc::free_camera_speed = 50.0f;
        globals::misc::long_neck = false;
        globals::misc::neck_length = 1.0f;
        globals::misc::no_fall_damage = false;
        globals::misc::click_tp = false;
        globals::misc::click_tp_key = 0;
        globals::misc::click_tp_mode = 0;
        globals::misc::spiderman = false;
        globals::misc::fly_swim = false;
        globals::misc::vehicle_fly = false;
        globals::misc::vehicle_fly_speed = 50.0f;

        globals::visuals::viewmodel::enabled = false;
        globals::visuals::viewmodel::fov = 70.0f;
        for (int i = 0; i < 3; i++) globals::visuals::viewmodel::offset[i] = 0.0f;

        globals::misc::no_send = false;
        globals::misc::jump_power_enabled = false;
        globals::misc::jump_power_key = 0;
        globals::misc::jump_power_mode = 0;
        globals::misc::jump_power_value = 50.0f;
        globals::misc::hip_height_enabled = false;
        globals::misc::hip_height_key = 0;
        globals::misc::hip_height_mode = 0;
        globals::misc::hip_height_value = 2.0f;
        globals::misc::gravity_enabled = false;
        globals::misc::gravity_key = 0;
        globals::misc::gravity_mode = 0;
        globals::misc::gravity_value = 196.2f;
        globals::misc::sit = false;
        globals::misc::platform_stand = false;
        globals::misc::jump_bug = false;
        globals::misc::jump_bug_key = 'K';
        globals::misc::jump_bug_mode = 0;
        globals::misc::no_rotate = false;
        globals::misc::no_clip = false;
        globals::misc::no_clip_key = 'C';
        globals::misc::no_clip_mode = 0;
        globals::misc::no_clip_method = 0;
        globals::misc::camera_offset_enabled = false;
        for (int i = 0; i < 3; i++) globals::misc::camera_offset[i] = 0.f;
        globals::misc::camera_zoom[0] = 0.5f; globals::misc::camera_zoom[1] = 400.f;
        globals::misc::fov = 70.f;
        globals::misc::zoom_enabled = false;
        globals::misc::zoom_key = 0;
        globals::misc::zoom_mode = 0;
        globals::misc::zoom_value = 20.0f;
        globals::misc::angles_enabled = false;
        globals::misc::angles_key = 'V';
        globals::misc::angles_mode = 0;
        globals::misc::pitch_base = 0;
        globals::misc::pitch_value = 0.0f;
        globals::misc::yaw_base = 0;
        globals::misc::yaw_value = 0.0f;
        globals::misc::spin_speed = 100.0f;
        globals::misc::upside_down = false;
        globals::misc::yaw_jitter = false;
        globals::misc::jitter_value = 0.0f;
        globals::misc::roll = false;
        globals::misc::anchor_on_bind = false;
        globals::misc::anchor_key = 'Y';
        globals::misc::anchor_mode = 0;
        globals::misc::waypoint_on_bind = false;
        globals::misc::waypoint_key = 'T';
        globals::misc::waypoint_mode = 0;
        globals::misc::waypoint_on_respawn = false;
        globals::misc::hit_sounds = false;
        globals::misc::hit_sounds_key = 0;
        globals::misc::hit_sounds_mode = 0;
        globals::misc::hit_sound_type = 0;
        globals::misc::death_sounds = false;
        globals::misc::death_sounds_key = 0;
        globals::misc::death_sounds_mode = 0;
        globals::misc::death_sound_type = 0;
        globals::misc::ping_spike = false;
        globals::misc::ping_spike_key = 0;
        globals::misc::ping_spike_mode = 0;
        globals::misc::ping_spike_value = 50.0f;
        globals::misc::desync = false;
        globals::misc::desync_key = 0;
        globals::misc::desync_mode = 0;
        globals::misc::tickrate_manipulation = false;
        globals::misc::tickrate_value = 240.0f;
        globals::visuals::masterswitch = true;
        globals::visuals::show_visuals = true;
        globals::visuals::fov = 70.f;
        globals::visuals::no_shadows = false;
        globals::visuals::no_shadows_key = 0;
        globals::visuals::no_shadows_mode = 0;
        globals::visuals::fullbright = false;
        globals::visuals::fullbright_key = 0;
        globals::visuals::fullbright_mode = 0;
        globals::visuals::glow_strength = 0.3f;
        for (int i = 0; i < 4; i++) {
            globals::visuals::ambient_color[i] = 1.f;
            globals::visuals::outdoor_ambient_color[i] = 1.f;
            globals::visuals::colorshift_top[i] = (i == 3 ? 1.f : 0.f);
            globals::visuals::colorshift_bottom[i] = (i == 3 ? 1.f : 0.f);
            globals::visuals::fog_color[i] = (i == 3 ? 1.f : 0.75f);
            globals::visuals::shadow_color[i] = (i == 3 ? 1.f : 0.f);
        }
        globals::visuals::brightness = 1.0f;
        globals::visuals::fog_density = 0.5f;
        globals::visuals::fog_start = 0.f;
        globals::visuals::fog_end = 10000.f;
        globals::visuals::exposure = 1.0f;
        globals::visuals::geographic_latitude = 23.5f;
        globals::visuals::no_fog = false;
        globals::visuals::no_fog_key = 0;
        globals::visuals::no_fog_mode = 0;
        globals::visuals::force_lighting = false;
        globals::visuals::celestial_bodies = true;
        globals::visuals::world_gravity = 196.2f;
        globals::visuals::jump_power = 50.0f;
        globals::visuals::walk_speed = 16.0f;
        globals::visuals::hip_height = 2.0f;
        globals::visuals::auto_jump = false;
        globals::visuals::auto_rotate = true;
        globals::visuals::time_of_day = 12.0f;
        globals::visuals::star_count = 3000;
        globals::visuals::sun_size = 11.f;
        globals::visuals::moon_size = 11.f;
        globals::visuals::third_person = false;
        globals::visuals::min_zoom = 0.5f;
        globals::visuals::max_zoom = 400.f;
        auto reset_player_settings = [](globals::visuals::player_settings_t* s) {
            *s = globals::visuals::player_settings_t();
            s->arrows_info = { 1, 1 };
        };

        reset_player_settings(&globals::visuals::enemies);
        reset_player_settings(&globals::visuals::friendlies);
        reset_player_settings(&globals::visuals::local_player);
        globals::visuals::client::corpse_chams = false;
        globals::visuals::client::corpse_chams_team_color = false;
        globals::visuals::client::corpse_names = false;
        globals::visuals::client::corpse_names_team_color = false;
        globals::visuals::client::corpse_type = 0;
        globals::visuals::client::corpse_shift = false;
        globals::visuals::client::corpse_max_dist = 5000.f;
        globals::visuals::client::enemy_highlight = false;
        globals::visuals::client::enemy_highlight_team_color = false;
        globals::visuals::client::friend_highlight = false;
        globals::visuals::client::friend_highlight_team_color = false;
        globals::visuals::client::colors::corpse_chams = IM_COL32(255, 255, 255, 255);
        globals::visuals::client::colors::corpse_names = IM_COL32(255, 255, 255, 255);
        globals::visuals::client::colors::enemy_highlight = IM_COL32(255, 0, 0, 255);
        globals::visuals::client::colors::friend_highlight = IM_COL32(0, 255, 0, 255);
        globals::visuals::options::visible_check = false;
        globals::visuals::options::forcefield_check = false;
        globals::visuals::options::alive_check = false;
        globals::visuals::options::tool_check = false;
        globals::visuals::options::ragdoll_check = false;
        globals::visuals::options::godded_check = false;
        globals::visuals::options::sort_by_status = false;
        globals::visuals::options::max_distance = 5000.f;
        globals::visuals::options::name_transform = 0;
        globals::visuals::options::max_size = 20.f;
        globals::visuals::options::font_type = 0;
        globals::visuals::options::render_outlines = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
        globals::visuals::options::global_outline_type = 1;
        globals::visuals::options::outline_thickness = 1.0f;
        globals::visuals::options::chams_outline_thickness = 1.0f;
        globals::visuals::options::skeleton_outline_thickness = 1.0f;
        globals::visuals::options::text_outline_thickness = 1.0f;
        globals::visuals::options::use_display_name = false;
        globals::visuals::options::combined_name = false;
        globals::visuals::options::box_inline = false;
        globals::visuals::options::box_type = 0;
        globals::visuals::options::bounding_type = 0;
        globals::visuals::options::static_on_death = false;
        globals::visuals::options::static_on_void = false;
        globals::visuals::options::gradient_bars = false;
        globals::visuals::options::bar_fold = false;
        globals::visuals::options::bar_size = 1.f;
        globals::visuals::options::value_percentages = false;
        globals::visuals::options::value_follow = false;
        globals::visuals::options::ignore_full = false;
        globals::visuals::options::head_type = 0;
        globals::visuals::options::head_mode = 0;
        globals::visuals::options::circular_head_dot = false;
        globals::visuals::options::show_none = false;
        globals::visuals::options::bounding_box_fill = false;
        globals::visuals::options::box_fill_gradient = false;
        globals::visuals::options::box_fill_gradient_spin = false;
        globals::visuals::options::box_glow = false;
        globals::visuals::options::healthbar_gradient = false;
        globals::visuals::options::healthbar_glow = false;
        globals::visuals::options::healthbar_text = false;
        globals::visuals::options::name_glow = false;
        globals::visuals::options::distance_glow = false;
        globals::visuals::options::chams_hit_impact = false;
        globals::visuals::options::chams_hit_impact_team_color = false;
        globals::visuals::options::chams_fresnel = false;
        globals::visuals::options::offscreen_arrows_mode = 0;
        globals::visuals::options::offscreen_arrows_radius = 150.f;
        globals::visuals::options::offscreen_arrows_size = 12.f;
        globals::visuals::options::offscreen_arrows_glow = false;
        globals::visuals::colors::box_fill[0] = IM_COL32(255, 255, 255, 100);
        globals::visuals::colors::box_fill[1] = IM_COL32(255, 255, 255, 100);
        globals::visuals::colors::healthbar[0] = IM_COL32(255, 0, 0, 255);
        globals::visuals::colors::healthbar[1] = IM_COL32(255, 255, 0, 255);
        globals::visuals::colors::healthbar[2] = IM_COL32(0, 255, 0, 255);
        globals::visuals::colors::healthbar_text = IM_COL32(255, 255, 255, 255);
        globals::visuals::colors::chams_hit_impact = IM_COL32(255, 255, 255, 255);
        globals::visuals::colors::chams_fresnel = IM_COL32(255, 255, 255, 255);
        globals::visuals::crosshair::enabled = false;
        globals::visuals::crosshair::team_color = false;
        globals::visuals::crosshair::spin = false;
        globals::visuals::crosshair::spin_speed = 1.0f;
        globals::visuals::crosshair::pulse = false;
        globals::visuals::crosshair::pulse_speed = 1.0f;
        globals::visuals::crosshair::origin = 0;
        globals::visuals::crosshair::lerp = false;
        globals::visuals::crosshair::lerp_speed = 10.0f;
        globals::visuals::crosshair::size = 10.0f;
        globals::visuals::crosshair::thickness = 1.0f;
        globals::visuals::crosshair::gap = 2.0f;
        globals::visuals::crosshair::dot = false;
        globals::visuals::crosshair::dot_size = 2.0f;
        globals::visuals::crosshair::outline = false;
        globals::visuals::crosshair::outline_thickness = 1.0f;
        globals::visuals::crosshair::t_style = false;
        globals::visuals::crosshair::sniper_crosshair = false;
        globals::visuals::crosshair::rainbow = false;
        globals::visuals::crosshair::rainbow_speed = 1.0f;
        globals::visuals::crosshair::color = IM_COL32(255, 255, 255, 255);
        globals::visuals::crosshair::dot_color = IM_COL32(255, 255, 255, 255);
        globals::visuals::crosshair::outline_color = IM_COL32(0, 0, 0, 255);

        globals::visuals::fov_circle::enabled = false;
        globals::visuals::fov_circle::team_color = false;
        globals::visuals::fov_circle::radius = 100.0f;
        globals::visuals::fov_circle::thickness = 1.0f;
        globals::visuals::fov_circle::color = IM_COL32(255, 255, 255, 255);
        globals::visuals::fov_circle::filled = false;
        globals::visuals::fov_circle::fill_color = IM_COL32(255, 255, 255, 50);
        globals::visuals::fov_circle::num_sides = 64;
        globals::visuals::fov_circle::origin = 0;
        globals::visuals::fov_circle::rainbow = false;
        globals::visuals::fov_circle::rainbow_speed = 1.0f;
        globals::visuals::fov_circle::dynamic = false;
    }

    std::filesystem::path get_path()
    {
        wchar_t path[MAX_PATH];
        std::filesystem::path p;
        std::string source;
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE, NULL, 0, path))) {
            p = path;
            p /= L"ud.gg";
            p /= L"configs";
            source = "Shell (Documents)";
        }
        else if (const char* userprofile = std::getenv("USERPROFILE")) {
            p = userprofile;
            p /= "Documents";
            p /= "ud.gg";
            p /= "configs";
            source = "Environment (USERPROFILE)";
        }
        else {
            p = std::filesystem::current_path() / "configs";
            source = "Current Directory";
        }

        std::error_code ec;
        bool created = false;
        if (!std::filesystem::exists(p, ec)) {
            std::filesystem::create_directories(p, ec);
            if (!ec) created = true;
        }
        if (ec || !std::filesystem::exists(p, ec)) {
            if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, path))) {
                p = path;
                p /= L"ud.gg";
                p /= L"configs";
                source = "Shell (AppData Fallback)";
                std::filesystem::create_directories(p, ec);
            }
        }

        static std::string last_source = "";
        static std::string last_path = "";
        if (source != last_source || p.string() != last_path) {
            logger->log<e_level::INFO>("Config path resolved via {} to: {}", source, p.string());
            last_source = source;
            last_path = p.string();
        }

        return p;
    }

    void save(const std::string& name)
    {
        init();
        auto path = get_path();
        if (path.empty()) {
            logger->log<e_level::WARN>("config::save: path is empty");
            return;
        }

        std::filesystem::path file_path = path / (name + ".cfg");
        std::ofstream file(file_path);
        if (!file.is_open()) {
            logger->log<e_level::ERORR>("config::save: failed to open file for writing: {}", file_path.string());
            return;
        }

        for (const auto& item : items) {
            file << item.name << " ";
            if (item.type == "bool") file << *(bool*)item.ptr;
            else if (item.type == "int") file << *(int*)item.ptr;
            else if (item.type == "float") file << *(float*)item.ptr;
            else if (item.type == "color") file << *(ImU32*)item.ptr;
            file << "\n";
        }
        file.close();


        std::filesystem::path wp_path = path / (name + ".wps");
        std::ofstream wp_file(wp_path);
        if (wp_file.is_open()) {
            for (const auto& wp : globals::misc::waypoints) {
                wp_file << wp.name << "|" << wp.pos.x << "|" << wp.pos.y << "|" << wp.pos.z << "|" << wp.keybind << "\n";
            }
            wp_file.close();
        }

        logger->log<e_level::INFO>("config::save: saved config to {}", file_path.string());
        refresh();
    }

    void load(const std::string& name)
    {
        init();
        auto path = get_path();
        if (path.empty()) {
            logger->log<e_level::WARN>("config::load: path is empty");
            return;
        }

        std::filesystem::path file_path = path / (name + ".cfg");
        std::ifstream file(file_path);
        if (!file.is_open()) {
            logger->log<e_level::ERORR>("config::load: failed to open file for reading: {}", file_path.string());
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string key;
            ss >> key;
            for (const auto& item : items) {
                if (item.name == key) {
                    if (item.type == "bool") ss >> *(bool*)item.ptr;
                    else if (item.type == "int") ss >> *(int*)item.ptr;
                    else if (item.type == "float") ss >> *(float*)item.ptr;
                    else if (item.type == "color") ss >> *(ImU32*)item.ptr;
                    break;
                }
            }
        }
        file.close();


        globals::misc::waypoints.clear();
        std::filesystem::path wp_path = path / (name + ".wps");
        std::ifstream wp_file(wp_path);
        if (wp_file.is_open()) {
            std::string wp_line;
            while (std::getline(wp_file, wp_line)) {
                std::stringstream wp_ss(wp_line);
                std::string wp_name, x_str, y_str, z_str, key_str;

                if (std::getline(wp_ss, wp_name, '|') &&
                    std::getline(wp_ss, x_str, '|') &&
                    std::getline(wp_ss, y_str, '|') &&
                    std::getline(wp_ss, z_str, '|') &&
                    std::getline(wp_ss, key_str, '|'))
                {
                    globals::misc::waypoint_t wp;
                    wp.name = wp_name;
                    wp.pos.x = std::stof(x_str);
                    wp.pos.y = std::stof(y_str);
                    wp.pos.z = std::stof(z_str);
                    wp.keybind = std::stoi(key_str);
                    globals::misc::waypoints.push_back(wp);
                }
            }
            wp_file.close();
        }

        logger->log<e_level::INFO>("config::load: loaded config from {}", file_path.string());
        current_config = name;
    }

    void remove(const std::string& name)
    {
        auto path = get_path();
        if (path.empty()) return;

        std::error_code ec;
        std::filesystem::remove(path / (name + ".cfg"), ec);
        std::filesystem::remove(path / (name + ".wps"), ec);
        refresh();
    }

    void refresh()
    {
        list.clear();
        auto path = get_path();
        if (path.empty()) {
            logger->log<e_level::WARN>("config::refresh: resolved path is empty");
            return;
        }

        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            logger->log<e_level::WARN>("config::refresh: directory does not exist: {}", path.string());
            return;
        }

        int total_files = 0;
        int cfg_files = 0;
        for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
            if (ec) {
                logger->log<e_level::ERORR>("config::refresh: directory_iterator error: {}", ec.message());
                break;
            }

            total_files++;
            auto entry_path = entry.path();
            if (!entry.is_regular_file()) continue;

            auto ext = entry_path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

            if (ext == ".cfg") {
                list.push_back(entry_path.stem().string());
                cfg_files++;
            }
        }

        static int last_count = -1;
        if (cfg_files != last_count) {
            std::sort(list.begin(), list.end());
            logger->log<e_level::INFO>("config::refresh: found {} .cfg files (out of {} total entries) in {}", cfg_files, total_files, path.string());
            last_count = cfg_files;
        }
    }

    void set_auto_load(const std::string& name)
    {
        auto path = get_path();
        if (path.empty()) return;

        std::ofstream file(path / "autoload.txt");
        if (file.is_open()) {
            file << name;
            file.close();
        }
    }

    void load_auto_config()
    {
        auto path = get_path();
        if (path.empty()) return;

        std::ifstream file(path / "autoload.txt");
        if (file.is_open()) {
            std::string name;
            file >> name;
            file.close();
            if (!name.empty()) {
                load(name);
            }
        }
    }
}

