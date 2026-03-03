#include "visuals.h"
#include "visualizer/visualizer.h"
#include "../../cache/cache.h"
#include "../misc/map_parser.h"
#include <algorithm>

const rbx::vector3_t visuals::corners[8] =
{
	{-1, -1, -1}, {1, -1, -1}, {-1, 1, -1},{1, 1, -1},
	{-1, -1, 1}, {1, -1, 1}, {-1, 1, 1}, {1, 1, 1}
};

#include <map>

void cheats::hook_visuals()
{
	if (!globals::visuals::masterswitch || !globals::visuals::show_visuals)
		return;

	std::shared_ptr<std::vector<cache::entity_t>> snapshot;
	cache::entity_t local_snapshot;
	{
		std::lock_guard<std::mutex> lock(cache::mtx);
		snapshot = cache::cached_players;
		local_snapshot = cache::cached_local_player;
	}
	rbx::vector2_t dims = globals::visualengine.get_dimensions();
	rbx::matrix4_t view = globals::visualengine.get_viewmatrix();
	ImVec2 window_offset = visualizer::get_window_offset();

	auto render_entity = [&](cache::entity_t& entity, globals::visuals::player_settings_t& settings) {
		if (globals::visuals::options::max_distance > 0.f && entity.distance > globals::visuals::options::max_distance)
			return;

		bool is_visible = cheats::misc::c_map_parser::get().is_visible(globals::get_camera().get_position(), entity.root_part.position, entity.instance.address);

		if (globals::visuals::options::wallcheck && !is_visible)
			return;

		bool is_dead = entity.health <= 0.f;

		if (globals::visuals::options::alive_check && is_dead)
		{
			if (!globals::visuals::client::corpse_chams && !globals::visuals::client::corpse_names)
				return;
		}

		if (!settings.enabled && !is_dead)
			return;

		ImU32 team_color = IM_COL32(
			static_cast<int>(entity.team_color.x * 255.f),
			static_cast<int>(entity.team_color.y * 255.f),
			static_cast<int>(entity.team_color.z * 255.f),
			255
		);

		ImVec2 c1, c2;
		bool valid;
		if (settings.bounding_box_type == 0)
			visualizer::box_calculations(entity, c1, c2, valid, dims, view, window_offset);
		else
			visualizer::static_box_calculations(entity, c1, c2, valid, dims, view, window_offset);

		if (is_dead)
		{
			if (globals::visuals::client::corpse_chams)
			{
				ImU32 final_corpse_chams_color = globals::visuals::client::corpse_chams_team_color ? team_color : globals::visuals::client::colors::corpse_chams;
				visualizer::chams(entity, final_corpse_chams_color, 0, false, 0, false, false, 0, 0, false, 0, dims, view, window_offset, 0, globals::visuals::client::corpse_type);
			}

			if (globals::visuals::client::corpse_names && valid)
			{
				ImU32 final_corpse_name_color = globals::visuals::client::corpse_names_team_color ? team_color : globals::visuals::client::colors::corpse_names;
				std::string name_to_draw = globals::visuals::options::use_display_name ? entity.display_name : entity.name;
				visualizer::name(c1, c2, name_to_draw, final_corpse_name_color, true, false);
			}

			return;
		}
		bool is_friendly = false;
		if (globals::custom_game::is_detected) {
			is_friendly = (entity.custom_team != 0 && entity.custom_team == globals::custom_game::local_team);
		} else {
			is_friendly = (entity.team_address != 0 && entity.team_address == cache::local_player_team);
		}
		if (is_friendly && globals::visuals::client::friend_highlight)
		{
			ImU32 final_highlight_color = globals::visuals::client::friend_highlight_team_color ? team_color : globals::visuals::client::colors::friend_highlight;
			visualizer::chams(entity, final_highlight_color, 0, false, 0, false, false, 0, 0, false, 0, dims, view, window_offset);
		}
		else if (!is_friendly && globals::visuals::client::enemy_highlight)
		{
			ImU32 final_highlight_color = globals::visuals::client::enemy_highlight_team_color ? team_color : globals::visuals::client::colors::enemy_highlight;
			visualizer::chams(entity, final_highlight_color, 0, false, 0, false, false, 0, 0, false, 0, dims, view, window_offset);
		}

		if (settings.chams)
		{
			ImU32 final_chams_color = settings.chams_team_color ? team_color : settings.colors.chams;

			if (settings.chams_health_based)
			{
				float health_ratio = std::clamp(entity.health / entity.maxhealth, 0.0f, 1.0f);
				int r = static_cast<int>(255.0f * (1.0f - health_ratio));
				int g = static_cast<int>(255.0f * health_ratio);
				final_chams_color = IM_COL32(r, g, 0, 255);
			}

			ImU32 final_flash_color = settings.chams_flash_team_color ? team_color : settings.colors.chams_flash;
			ImU32 final_hit_color = globals::visuals::options::chams_hit_impact_team_color ? team_color : globals::visuals::colors::chams_hit_impact;
			visualizer::chams(entity, final_chams_color, settings.colors.chams_fresnel, settings.chams_glow, settings.colors.chams_glow, false, settings.chams_flash, settings.chams_flash_speed, final_flash_color, settings.chams_fade, settings.chams_fade_speed, dims, view, window_offset, final_hit_color, settings.chams_texture, (float)settings.chams_glow_intensity);
		}

		if (settings.movement_trails)
		{
			ImU32 final_trail_color = settings.colors.chams; // Using chams color for trails
			visualizer::movement_trails(entity, final_trail_color, 2.0f, 50, dims, view, window_offset);
		}

		if (settings.view_angle_lines)
		{
			ImU32 final_view_color = settings.colors.chams; // Using chams color for view lines
			visualizer::view_angle_lines(entity, final_view_color, 15.0f, 1.5f, dims, view, window_offset);
		}

		ImU32 final_box_color = settings.box_team_color ? team_color : settings.colors.box;
		if (settings.visibility_colors)
			final_box_color = is_visible ? settings.colors.box_vis : settings.colors.box_invis;

		if (settings.sound_esp)
		{
			ImU32 final_sound_esp_color = settings.sound_esp_team_color ? team_color : settings.colors.sound_esp;
			ImU32 final_sound_esp_glow_color = settings.sound_esp_team_color ? team_color : settings.colors.sound_esp_glow;
			visualizer::sound_esp(entity, final_sound_esp_color, settings.sound_esp_glow, final_sound_esp_glow_color, settings.sound_esp_radius, settings.sound_esp_speed, dims, view, window_offset);
		}

		if (settings.footprints)
		{
			ImU32 final_footprints_color = settings.footprints_team_color ? team_color : settings.colors.footprints;
			ImU32 final_footprints_glow_color = settings.footprints_team_color ? team_color : settings.colors.footprints_glow;
			visualizer::footprints(entity, final_footprints_color, settings.footprints_glow, final_footprints_glow_color, settings.footprints_radius, settings.footprints_speed, dims, view, window_offset);
		}

		if (!valid)
		{
			if (settings.arrows)
			{
				std::string name_to_draw = globals::visuals::options::use_display_name ? entity.display_name : entity.name;
				ImU32 final_arrows_color = settings.arrows_team_color ? team_color : settings.colors.arrows;
				visualizer::offscreen_arrows(entity.root_part.position, final_arrows_color, settings.arrows_glow, settings.colors.arrows_glow, settings.arrows_size, settings.arrows_position, name_to_draw, entity.distance, settings.arrows_info, dims, view, window_offset);
			}

			return;
		}

		if (settings.bounding_box)
		{
			ImU32 final_box_fill[2] = { settings.colors.box_fill[0], settings.colors.box_fill[1] };
			if (settings.box_fill_team_color)
			{
				final_box_fill[0] = (team_color & 0x00FFFFFF) | (settings.colors.box_fill[0] & 0xFF000000);
				final_box_fill[1] = (team_color & 0x00FFFFFF) | (settings.colors.box_fill[1] & 0xFF000000);
			}

			if (settings.bounding_box_style == 0)
				visualizer::bounding_box(c1, c2, final_box_color, settings.box_glow, settings.colors.box_glow, settings.box_fill, settings.box_fill_gradient, settings.box_fill_gradient_spin, settings.box_fill_gradient_speed, settings.box_fill_gradient_style, final_box_fill);
			else if (settings.bounding_box_style == 1)
				visualizer::corner_box(c1, c2, final_box_color, settings.corner_box_length, settings.box_glow, settings.colors.box_glow, settings.box_fill, settings.box_fill_gradient, settings.box_fill_gradient_spin, settings.box_fill_gradient_speed, settings.box_fill_gradient_style, final_box_fill);
			else if (settings.bounding_box_style == 2)
				visualizer::box_3d(entity, final_box_color, settings.bounding_box_type == 1, dims, view, window_offset);
		}

		if (settings.healthbar)
		{
			ImU32 final_health_color = settings.healthbar_team_color ? team_color : settings.colors.healthbar;
			ImU32 final_health_text_color = settings.health_value_team_color ? team_color : settings.colors.healthbar_text;
			visualizer::healthbar(entity.instance.address, c1, c2, entity.health, entity.maxhealth, final_health_color, 1.0f, globals::visuals::options::bar_size, IM_COL32(0, 0, 0, 255), settings.healthbar_glow, settings.colors.healthbar_glow, settings.healthbar_gradient, settings.colors.healthbar_gradient, settings.healthbar_text, final_health_text_color, settings.healthbar_lerp, settings.bounding_box_style == 1);
		}

		if (settings.tracers)
		{
			ImU32 final_tracers_color = settings.tracers_team_color ? team_color : settings.colors.tracers;
			visualizer::tracers(entity.root_part.position, final_tracers_color, settings.tracers_thickness, settings.tracers_origin, dims, view, window_offset);
		}

		if (settings.mesh_chams)
		{
			ImU32 final_mesh_chams_color = settings.mesh_chams_team_color ? team_color : settings.colors.mesh_chams;
			visualizer::mesh_chams(entity, final_mesh_chams_color, settings.mesh_chams_glow, settings.colors.mesh_chams_glow, settings.chams_fade, settings.chams_fade_speed, dims, view, window_offset);
		}

		if (settings.skeleton)
		{
			ImU32 final_skeleton_color = settings.skeleton_team_color ? team_color : settings.colors.skeleton;
			visualizer::skeleton(entity, final_skeleton_color, settings.skeleton_glow, settings.colors.skeleton_glow, dims, view, window_offset);
		}

		if (settings.name)
		{
			std::string name_to_draw;

			if (globals::visuals::options::combined_name)
				name_to_draw = entity.display_name + "(@" + entity.name + ")";
			else
				name_to_draw = globals::visuals::options::use_display_name ? entity.display_name : entity.name;

			if (globals::visuals::options::name_transform == 1)
				std::transform(name_to_draw.begin(), name_to_draw.end(), name_to_draw.begin(), ::toupper);
			else if (globals::visuals::options::name_transform == 2)
				std::transform(name_to_draw.begin(), name_to_draw.end(), name_to_draw.begin(), ::tolower);

			ImU32 final_name_color = settings.name_team_color ? team_color : settings.colors.name;
			if (settings.visibility_colors)
				final_name_color = is_visible ? settings.colors.name_vis : settings.colors.name_invis;

			visualizer::name(c1, c2, name_to_draw, final_name_color, true, settings.name_glow);
		}

		if (settings.distance)
		{
			ImU32 final_dist_color = settings.distance_team_color ? team_color : settings.colors.distance;
			visualizer::distance(c1, c2, entity.distance, final_dist_color, true, settings.distance_glow);
		}

		if (settings.state_flags || settings.rig_flags)
		{
			std::string state_str = "";
			if (settings.state_flags)
			{
				uint8_t state = entity.humanoid.get_state();
				switch (state)
				{
				case 0: state_str = "Falling"; break;
				case 1: state_str = "Ragdoll"; break;
				case 2: state_str = "GettingUp"; break;
				case 3: state_str = "Jumping"; break;
				case 4: state_str = "Swimming"; break;
				case 5: state_str = "Freefall"; break;
				case 6: state_str = "Flying"; break;
				case 7: state_str = "Landed"; break;
				case 8: state_str = "Running"; break;
				case 9: state_str = "RunningNoPhysics"; break;
				case 10: state_str = "StrafingNoPhysics"; break;
				case 11: state_str = "Climbing"; break;
				case 12: state_str = "Seated"; break;
				case 13: state_str = "PlatformStanding"; break;
				case 14: state_str = "Dead"; break;
				case 15: state_str = "Physics"; break;
				case 16: state_str = "None"; break;
				default: state_str = "Unknown"; break;
				}
			}

			std::string rig_str = "";
			if (settings.rig_flags)
			{
				rig_str = (entity.rig_type == 1) ? "R15" : "R6";
			}

			visualizer::esp_flags(c1, c2, state_str, rig_str, final_box_color, settings.bounding_box_style == 1);
		}

		if (settings.head_dot)
		{
			ImU32 final_head_dot_color = settings.head_dot_team_color ? team_color : settings.colors.head_dot;
			visualizer::head_visuals(entity, final_head_dot_color, settings.head_dot_glow, settings.colors.head_dot_glow, dims, view, window_offset);
		}

		if (settings.tool)
		{
			ImU32 final_tool_color = settings.tool_team_color ? team_color : settings.colors.tool;
			visualizer::tool(c1, c2, entity.tool_name, final_tool_color, settings.distance);
		}
	};

	ImU32 local_team_color = IM_COL32(
		static_cast<int>(local_snapshot.team_color.x * 255.f),
		static_cast<int>(local_snapshot.team_color.y * 255.f),
		static_cast<int>(local_snapshot.team_color.z * 255.f),
		255
	);

	render_entity(local_snapshot, globals::visuals::local_player);

	for (cache::entity_t& entity : *snapshot)
	{
		bool is_friendly = (entity.team_address != 0 && entity.team_address == cache::local_player_team);
		auto& settings = is_friendly ? globals::visuals::friendlies : globals::visuals::enemies;

		render_entity(entity, settings);
	}

	ImU32 final_crosshair_color = globals::visuals::crosshair::team_color ? local_team_color : globals::visuals::crosshair::color;
	visualizer::crosshair(dims, final_crosshair_color);
	visualizer::fov_circle(dims, globals::visuals::fov_circle::color);
}

