#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <map>
#include <shared_mutex>
#include <string>

#include "features/sdk/sdk.h"
#include "memory/memory.h"
#include "features/cache/cache.h"
#include "render/render.h"

namespace globals
{
	inline std::atomic<bool> running = true;
	inline HWND roblox_window = nullptr;
	inline std::shared_mutex globals_mtx;
	inline rbx::instance_t datamodel{};
	inline rbx::visualengine_t visualengine{};
	inline rbx::visualengine_t renderview{};
	inline rbx::instance_t players{};
	inline rbx::instance_t local_player{};
	inline rbx::instance_t workspace{};
	inline rbx::instance_t lighting{};
	inline rbx::camera_t camera{};
	inline rbx::instance_t run_service{};
    inline rbx::instance_t get_local_player() {
        std::shared_lock lock(globals_mtx);
        return local_player;
    }

    inline rbx::camera_t get_camera() {
        std::shared_lock lock(globals_mtx);
        return camera;
    }

    inline rbx::visualengine_t get_visualengine() {
        std::shared_lock lock(globals_mtx);
        return visualengine;
    }

    inline rbx::visualengine_t get_renderview() {
        std::shared_lock lock(globals_mtx);
        return renderview;
    }

    inline rbx::instance_t get_workspace() {
        std::shared_lock lock(globals_mtx);
        return workspace;
    }

    inline rbx::instance_t get_lighting() {
        std::shared_lock lock(globals_mtx);
        return lighting;
    }

	inline std::map<std::uint64_t, int> player_overrides;
	inline std::mutex player_overrides_mtx;

	namespace settings
	{
		inline bool teamcheck;
		inline bool clientcheck;
		inline bool streamproof;
		inline float dpi_scaling = 1.0f;

		inline int performance_def = 0;
		inline bool low_latency = false;
		inline int rendering_quality = 10;
		inline bool show_fps = false;
		inline bool watermark = false;
	}

	namespace custom_game
	{
		inline bool is_detected = false;
		inline int local_team = 0; // 0: none, 1: terrorist, 2: counter-terrorist
		inline rbx::instance_t terrorist_folder{};
		inline rbx::instance_t counter_terrorist_folder{};
	}

	namespace aimbot
	{
		inline bool enabled = false;
		inline int keybind = 0;
		inline int keybind_mode = 0;
		inline int method = 0;
		inline float fov = 22.8f;
		inline int radius = 0;
		inline bool deadzone = false;
		inline float deadzone_value = 1.8f;
		inline bool dynamic_fov = false;
		inline float dynamic_fov_value = 26.5f;
		inline bool smoothing = false;
		inline float smoothing_values[2] = { 30.2f, 31.0f };
		inline std::vector<int> hitboxes = { 1, 1 };
		inline bool random_hitbox = false;
		inline float random_hitbox_value = 500.f;
		inline bool root_on_void = false;
		inline int hitscan_type = 0;
		inline bool wallcheck = false;
		inline bool auto_wall = false;
		inline std::vector<int> aiming_checks = { 1, 1, 1, 1, 0, 0, 0, 0 };

		inline ImVec2 target_screen_pos = { 0, 0 };
		inline bool has_target = false;

		namespace target_strafe
		{
			inline bool enabled = false;
			inline float speed = 30.0f;
			inline float radius = 10.0f;
			inline float height = 10.0f;
		}

		namespace misc
		{
			inline bool team_check = true;
			inline bool visible_check = true;
			inline bool forcefield_check = true;
			inline bool alive_check = true;
			inline bool tool_check = false;
			inline bool ragdoll_check = false;
			inline bool godded_check = false;
			inline bool melee_check = false;
			inline bool readjustment = false;
			inline int readjustment_key = 0;
			inline int readjustment_mode = 0;
			inline bool cursor_offset = false;
			inline float cursor_offset_values[2] = { 0.f, 0.f };
		}

		namespace trigger
		{
			inline bool enabled = false;
			inline int keybind = 0;
			inline int keybind_mode = 0;
			inline float delay = 5.0f;
			inline float interval = 75.0f;
			inline std::vector<int> hitboxes = { 1 };
			inline float hit_chance = 100.0f;
			inline float hitbox_scale = 1.0f;
		}

		namespace prediction
		{
			inline bool enabled = false;
			inline bool velocity_prediction = false;
			inline bool multiplier = false;
			inline float multiplier_values[2] = { 0.5f, 0.5f };
			inline float x_multiplier = 1.0f;
			inline float y_multiplier = 1.0f;
			inline bool bullet_speed = false;
			inline float bullet_speed_value = 0.5f;
			inline bool bullet_gravity = false;
			inline float bullet_gravity_value = 0.5f;
		}

	}

	namespace misc
	{
		inline bool speed_hack = false;
		inline int speed_key = 'H';
		inline int speed_mode = 0;
		inline int speed_method = 0;
		inline float speed_value = 16.0f;

		inline bool fly_hack = false;
		inline int fly_key = 'J';
		inline int fly_mode = 0;
		inline int fly_method = 0;
		inline float fly_speed = 50.0f;

		inline bool free_camera = false;
		inline int free_camera_key = 'C';
		inline int free_camera_mode = 0;
		inline int free_camera_method = 0;
		inline float free_camera_speed = 50.0f;

		inline bool long_neck = false;
		inline float neck_length = 1.0f;

		inline bool no_fall_damage = false;
		inline bool click_tp = false;
		inline int click_tp_key = 0;
		inline int click_tp_mode = 0;
		inline bool spiderman = false;
		inline bool fly_swim = false;
		inline bool vehicle_fly = false;
		inline float vehicle_fly_speed = 50.0f;
		inline int no_clip_method = 0;

		inline bool ideal_peak = false;
		inline int ideal_peak_key = 0;
		inline int ideal_peak_mode = 0;
		inline rbx::vector3_t ideal_peak_pos = { 0, 0, 0 };
		inline bool has_ideal_peak = false;

		inline bool no_send = false;
		inline bool anti_afk = false;

		struct waypoint_t {
			std::string name;
			rbx::vector3_t pos;
			int keybind = 0;
		};
		inline std::vector<waypoint_t> waypoints;
		inline char waypoint_name_buffer[64] = "";
		inline int selected_waypoint = -1;

		inline bool jump_power_enabled = false;
		inline int jump_power_key = 0;
		inline int jump_power_mode = 0;
		inline float jump_power_value = 50.0f;


		inline bool hip_height_enabled = false;
		inline int hip_height_key = 0;
		inline int hip_height_mode = 0;
		inline float hip_height_value = 2.0f;

		inline bool gravity_enabled = false;
		inline int gravity_key = 0;
		inline int gravity_mode = 0;
		inline float gravity_value = 196.2f;

		inline bool sit = false;

		inline bool platform_stand = false;

		inline bool jump_bug = false;
		inline int jump_bug_key = 'K';
		inline int jump_bug_mode = 0;

		inline bool no_rotate = false;

		inline bool recoil_reduction = false;
		inline float recoil_reduction_value = 100.0f;

		inline bool no_clip = false;
		inline int no_clip_key = 'C';
		inline int no_clip_mode = 0;
		inline bool camera_offset_enabled = false;
		inline float camera_offset[3] = { 0.f, 0.f, 0.f };
		inline float camera_zoom[2] = { 0.5f, 400.f };
		inline float fov = 70.f;

		inline bool zoom_enabled = false;
		inline int zoom_key = 0;
		inline int zoom_mode = 0;
		inline float zoom_value = 20.0f;
		inline bool angles_enabled = false;
		inline int angles_key = 'V';
		inline int angles_mode = 0;
		inline int pitch_base = 0;
		inline float pitch_value = 0.0f;
		inline int yaw_base = 0;
		inline float yaw_value = 0.0f;
		inline float spin_speed = 100.0f;
		inline bool upside_down = false;

		inline bool yaw_jitter = false;
		inline float jitter_value = 0.0f;

		inline bool roll = false;
		inline bool anchor_on_bind = false;
		inline int anchor_key = 'Y';
		inline int anchor_mode = 0;

		inline bool waypoint_on_bind = false;
		inline int waypoint_key = 'T';
		inline int waypoint_mode = 0;

		inline bool waypoint_on_respawn = false;

		inline bool hit_sounds = false;
		inline int hit_sounds_key = 0;
		inline int hit_sounds_mode = 0;
		inline int hit_sound_type = 0;

		inline bool death_sounds = false;
		inline int death_sounds_key = 0;
		inline int death_sounds_mode = 0;
		inline int death_sound_type = 0;

		inline bool ping_spike = false;
		inline int ping_spike_key = 0;
		inline int ping_spike_mode = 0;
		inline float ping_spike_value = 50.0f;
		inline bool desync = false;
		inline int desync_key = 0;
		inline int desync_mode = 0;

		inline bool tickrate_manipulation = false;
		inline float tickrate_value = 240.0f;

		inline std::uint64_t target_entity_address = 0;
	}

	namespace visuals
	{
		namespace viewmodel
		{
			inline bool enabled = false;
			inline float fov = 70.0f;
			inline float offset[3] = { 0.0f, 0.0f, 0.0f };
		}
		inline bool masterswitch = true;

		inline bool show_visuals = true;

		inline float fov = 70.f;
		inline bool no_shadows = false;
		inline int no_shadows_key = 0;
		inline int no_shadows_mode = 0;

		inline bool fullbright = false;
		inline int fullbright_key = 0;
		inline int fullbright_mode = 0;

		inline float glow_strength = 0.3f;

		inline float ambient_color[4] = { 1.f, 1.f, 1.f, 1.f };
		inline float outdoor_ambient_color[4] = { 1.f, 1.f, 1.f, 1.f };
		inline float colorshift_top[4] = { 0.f, 0.f, 0.f, 1.f };
		inline float colorshift_bottom[4] = { 0.f, 0.f, 0.f, 1.f };
		inline float brightness = 1.0f;
		inline float fog_density = 0.5f;
		inline float fog_color[4] = { 0.75f, 0.75f, 0.75f, 1.f };
		inline float shadow_color[4] = { 0.f, 0.f, 0.f, 1.f };
		inline float fog_start = 0.f;
		inline float fog_end = 10000.f;
		inline float exposure = 1.0f;
		inline float geographic_latitude = 23.5f;
		inline bool no_fog = false;
		inline int no_fog_key = 0;
		inline int no_fog_mode = 0;

		inline bool map_debug = false;

		inline bool force_lighting = false;

		namespace atmosphere
		{
			inline bool enabled = false;
			inline float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			inline float decay[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			inline float density = 0.3f;
			inline float glare = 0.0f;
			inline float haze = 0.0f;
			inline float offset = 0.0f;
		}

		namespace bloom
		{
			inline bool enabled = false;
			inline float intensity = 1.0f;
			inline float size = 24.0f;
			inline float threshold = 2.0f;
		}

		namespace sunrays
		{
			inline bool enabled = false;
			inline float intensity = 0.01f;
			inline float spread = 0.1f;
		}

		namespace color_correction
		{
			inline bool enabled = false;
			inline float brightness = 0.0f;
			inline float contrast = 0.0f;
			inline float saturation = 0.0f;
			inline float tint_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		}

		namespace depth_of_field
		{
			inline bool enabled = false;
			inline float density = 0.1f;
			inline float focus_distance = 10.0f;
			inline float in_focus_radius = 30.0f;
			inline float near_intensity = 0.75f;
		}

		inline float environment_diffuse_scale = 0.3f;
		inline float environment_specular_scale = 1.0f;

		namespace terrain
		{
			inline bool enabled = false;
			inline float grass_length = 0.1f;
			inline float water_transparency = 0.5f;
			inline float water_reflectance = 0.5f;
			inline float water_wave_size = 0.5f;
			inline float water_wave_speed = 0.5f;
			inline float water_color[4] = { 0.0f, 0.5f, 1.0f, 1.0f };
		}

		inline bool celestial_bodies = true;

		inline float world_gravity = 196.2f;
		inline float jump_power = 50.0f;
		inline float walk_speed = 16.0f;
		inline float hip_height = 2.0f;

		inline bool auto_jump = false;

		inline bool auto_rotate = true;

		inline float time_of_day = 12.0f;
		inline int star_count = 3000;
		inline float sun_size = 11.f;
		inline float moon_size = 11.f;

		inline bool third_person = false;
		inline float min_zoom = 0.5f;
		inline float max_zoom = 400.f;

		namespace fov_circle
		{
			inline bool enabled = false;
			inline bool team_color = false;
			inline float radius = 100.0f;
			inline float thickness = 1.0f;
			inline ImU32 color = IM_COL32(255, 255, 255, 255);
			inline bool filled = false;
			inline ImU32 fill_color = IM_COL32(255, 255, 255, 50);
			inline int num_sides = 64;
			inline int origin = 0;
			inline bool rainbow = false;
			inline float rainbow_speed = 1.0f;
			inline bool dynamic = false;
		}

		struct player_settings_t
		{
			bool enabled = false;
			bool name = false;
			bool name_glow = false;
			bool bounding_box = false;
			int bounding_box_type = 0;
			int bounding_box_style = 0;
			float corner_box_length = 0.3f;
			bool box_glow = false;
			bool healthbar = false;
			bool healthbar_glow = false;
			bool health_value = false;
			bool arrows = false;
			bool arrows_glow = false;
			float arrows_size = 12.f;
			float arrows_position = 40.f;
			int arrow_type = 0;
			float arrows_max_dist = 5000.f;
			std::vector<int> arrows_info = { 1, 1 };
			bool head_dot = false;
			bool head_dot_glow = false;
			bool tool = false;
			bool distance = false;
			bool distance_glow = false;
			bool state_flags = false;
			bool rig_flags = false;
			bool chams = false;
			bool chams_glow = false;
			bool chams_health_based = false;
			int chams_texture = 0;
			bool chams_flash = false;
			float chams_flash_speed = 1.0f;
			bool chams_fade = false;
			float chams_fade_speed = 1.0f;
			bool mesh_chams = false;
			bool mesh_chams_glow = false;
			bool skeleton = false;
			bool skeleton_glow = false;

			bool movement_trails = false;
			bool movement_trails_glow = false;
			float movement_trails_thickness = 1.0f;
			float movement_trails_length = 1.0f;

			bool view_angle_lines = false;
			bool view_angle_lines_glow = false;
			float view_angle_lines_length = 10.0f;
			float view_angle_lines_thickness = 1.0f;

			float chams_glow_intensity = 100.0f;

			bool sound_esp = false;
			bool sound_esp_glow = false;
			float sound_esp_radius = 5.0f;
			float sound_esp_speed = 1.0f;

			bool footprints = false;
			bool footprints_glow = false;
			float footprints_radius = 1.0f;
			float footprints_speed = 1.0f;

			bool tracers = false;
			float tracers_thickness = 1.0f;
			int tracers_origin = 0;

			bool box_fill = false;
			bool box_fill_gradient = false;
			bool box_fill_gradient_spin = false;
			float box_fill_gradient_speed = 2.0f;
			int box_fill_gradient_style = 0;

			bool healthbar_gradient = false;
			bool healthbar_text = false;
			bool healthbar_lerp = true;

			bool box_team_color = false;
			bool chams_team_color = false;
			bool arrows_team_color = false;
			bool name_team_color = false;
			bool head_dot_team_color = false;
			bool tool_team_color = false;
			bool distance_team_color = false;
			bool tracers_team_color = false;
			bool skeleton_team_color = false;
			bool movement_trails_team_color = false;
			bool view_angle_lines_team_color = false;
			bool healthbar_team_color = false;
			bool sound_esp_team_color = false;
			bool footprints_team_color = false;
			bool mesh_chams_team_color = false;
			bool box_fill_team_color = false;
			bool health_value_team_color = false;
			bool chams_flash_team_color = false;

			bool visibility_colors = false;

			struct colors_struct
			{
				ImU32 name = IM_COL32(255, 255, 255, 255);
				ImU32 name_vis = IM_COL32(0, 255, 0, 255);
				ImU32 name_invis = IM_COL32(255, 0, 0, 255);

				ImU32 name_glow = IM_COL32(255, 255, 255, 255);
				ImU32 box = IM_COL32(255, 255, 255, 255);
				ImU32 box_vis = IM_COL32(0, 255, 0, 255);
				ImU32 box_invis = IM_COL32(255, 0, 0, 255);

				ImU32 box_glow = IM_COL32(255, 255, 255, 255);
				ImU32 box_fill[2] = { IM_COL32(255, 255, 255, 100), IM_COL32(255, 255, 255, 100) };
				ImU32 healthbar = IM_COL32(255, 255, 255, 255);
				ImU32 healthbar_glow = IM_COL32(255, 255, 255, 255);
				ImU32 healthbar_text = IM_COL32(255, 255, 255, 255);
				ImU32 healthbar_gradient[3] = { IM_COL32(255, 0, 0, 255), IM_COL32(255, 255, 0, 255), IM_COL32(0, 255, 0, 255) };
				ImU32 arrows = IM_COL32(255, 255, 255, 255);
				ImU32 arrows_glow = IM_COL32(255, 255, 255, 255);
				ImU32 head_dot = IM_COL32(255, 255, 255, 255);
				ImU32 head_dot_glow = IM_COL32(255, 255, 255, 255);
				ImU32 tool = IM_COL32(255, 255, 255, 255);
				ImU32 distance = IM_COL32(255, 255, 255, 255);
				ImU32 distance_glow = IM_COL32(255, 255, 255, 255);
				ImU32 chams = IM_COL32(255, 255, 255, 255);
				ImU32 chams_glow = IM_COL32(255, 255, 255, 255);
				ImU32 chams_flash = IM_COL32(255, 0, 0, 255);
				ImU32 chams_fresnel = IM_COL32(255, 255, 255, 255);
				ImU32 mesh_chams = IM_COL32(255, 255, 255, 255);
				ImU32 mesh_chams_glow = IM_COL32(255, 255, 255, 255);
				ImU32 skeleton = IM_COL32(255, 255, 255, 255);
				ImU32 skeleton_glow = IM_COL32(255, 255, 255, 255);
				ImU32 movement_trails = IM_COL32(255, 255, 255, 255);
				ImU32 movement_trails_glow = IM_COL32(255, 255, 255, 255);
				ImU32 view_angle_lines = IM_COL32(255, 255, 255, 255);
				ImU32 view_angle_lines_glow = IM_COL32(255, 255, 255, 255);
				ImU32 sound_esp = IM_COL32(255, 255, 255, 255);
				ImU32 sound_esp_glow = IM_COL32(255, 255, 255, 255);
				ImU32 footprints = IM_COL32(255, 255, 255, 255);
				ImU32 footprints_glow = IM_COL32(255, 255, 255, 255);
				ImU32 tracers = IM_COL32(255, 255, 255, 255);
			} colors;
		};

		inline player_settings_t enemies;
		inline player_settings_t friendlies;
		inline player_settings_t local_player;

		namespace client
		{
			inline bool corpse_chams = false;
			inline bool corpse_chams_team_color = false;

			inline bool corpse_names = false;
			inline bool corpse_names_team_color = false;

			inline int corpse_type = 0;
			inline bool corpse_shift = false;

			inline float corpse_max_dist = 5000.f;
			inline bool enemy_highlight = false;
			inline bool enemy_highlight_team_color = false;

			inline bool friend_highlight = false;
			inline bool friend_highlight_team_color = false;

			namespace colors
			{
				inline ImU32 corpse_chams = IM_COL32(255, 255, 255, 255);
				inline ImU32 corpse_names = IM_COL32(255, 255, 255, 255);
				inline ImU32 enemy_highlight = IM_COL32(255, 0, 0, 255);
				inline ImU32 friend_highlight = IM_COL32(0, 255, 0, 255);
			}
		}

		namespace options
		{
			inline bool visible_check = false;
			inline bool wallcheck = false;
			inline bool forcefield_check = false;
			inline bool alive_check = false;
			inline bool tool_check = false;
			inline bool ragdoll_check = false;
			inline bool godded_check = false;
			inline bool sort_by_status = false;

			inline float max_distance = 5000.f;
			inline int name_transform = 0;
			inline float max_size = 20.f;
			inline int font_type = 0;

			inline std::vector<int> render_outlines = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
			inline int global_outline_type = 1;

			inline float outline_thickness = 1.0f;
			inline float chams_outline_thickness = 1.0f;
			inline float skeleton_outline_thickness = 1.0f;
			inline float text_outline_thickness = 1.0f;
			inline bool use_display_name = false;
			inline bool combined_name = false;
			inline bool box_inline = false;
			inline int box_type = 0;
			inline int bounding_type = 0;

			inline bool static_on_death = false;
			inline bool static_on_void = false;
			inline bool gradient_bars = false;
			inline bool bar_fold = false;
			inline float bar_size = 1.f;

			inline bool value_percentages = false;
			inline bool value_follow = false;
			inline bool ignore_full = false;
			inline bool circular_head_dot = false;
			inline int head_type = 0; // 0: Dot, 1: Hexagon
			inline int head_mode = 0; // 0: Static, 1: Bounding
			inline float head_size_limit = 5.0f;
			inline float head_static_size = 1.2f;
			inline bool show_none = false;

			inline bool bounding_box_fill = false;
			inline bool box_fill_gradient = false;
			inline bool box_fill_gradient_spin = false;
			inline bool box_glow = false;
			inline bool healthbar_gradient = false;
			inline bool healthbar_glow = false;
			inline bool healthbar_text = false;
			inline bool name_glow = false;
			inline bool distance_glow = false;
			inline bool chams_hit_impact = false;
			inline bool chams_hit_impact_team_color = false;
			inline bool chams_fresnel = false;
			inline int offscreen_arrows_mode = 0;
			inline float offscreen_arrows_radius = 150.f;
			inline float offscreen_arrows_size = 12.f;
			inline bool offscreen_arrows_glow = false;
		}

		namespace colors
		{
			inline ImU32 box_fill[2] = { IM_COL32(255, 255, 255, 100), IM_COL32(255, 255, 255, 100) };
			inline ImU32 healthbar[3] = { IM_COL32(255, 0, 0, 255), IM_COL32(255, 255, 0, 255), IM_COL32(0, 255, 0, 255) };
			inline ImU32 healthbar_text = IM_COL32(255, 255, 255, 255);
			inline ImU32 chams_hit_impact = IM_COL32(255, 255, 255, 255);
			inline ImU32 chams_fresnel = IM_COL32(255, 255, 255, 255);
		}

		namespace crosshair
		{
			inline bool enabled = false;
			inline bool team_color = false;
			inline bool spin = false;
			inline float spin_speed = 1.0f;
			inline bool pulse = false;
			inline float pulse_speed = 1.0f;
			inline int origin = 0;
			inline bool lerp = false;
			inline float lerp_speed = 10.0f;
			inline float size = 10.0f;
			inline float thickness = 1.0f;
			inline float gap = 2.0f;
			inline bool dot = false;
			inline float dot_size = 2.0f;
			inline bool dot_outline = false;
			inline bool outline = false;
			inline float outline_thickness = 1.0f;
			inline bool t_style = false;
			inline bool sniper_crosshair = false;
			inline bool rainbow = false;
			inline float rainbow_speed = 1.0f;
			inline ImU32 color = IM_COL32(255, 255, 255, 255);
			inline ImU32 dot_color = IM_COL32(255, 255, 255, 255);
			inline ImU32 outline_color = IM_COL32(0, 0, 0, 255);
		}
	}
}

