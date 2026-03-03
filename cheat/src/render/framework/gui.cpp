#include "settings/functions.h"
#include "../../features/settings/config.h"
#include <Windows.h>
#include <cstdio>
#include <functional>
#include <algorithm>
#include <wininet.h>
#include <d3d11.h>
#include <thread>
#include <mutex>
#include <set>
#include "globals.h"
#include "render/render.h"
#include "../../features/cheats/misc/map_parser.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../../ext/stb-master/stb_image.h"

#pragma comment(lib, "wininet.lib")

static std::vector<uint8_t> download_url(const std::string& url) {
    std::vector<uint8_t> content;
    HINTERNET hInternet = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return content;

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return content;
    }

    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        content.insert(content.end(), buffer, buffer + bytesRead);
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return content;
}

static ID3D11ShaderResourceView* create_texture_from_memory(const std::vector<uint8_t>& data) {
    int width, height, channels;
    unsigned char* image_data = stbi_load_from_memory(data.data(), (int)data.size(), &width, &height, &channels, 4);
    if (!image_data) return nullptr;

    ID3D11Texture2D* texture = nullptr;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource{};
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = width * 4;

    HRESULT hr = render->detail_ptr->device->CreateTexture2D(&desc, &subResource, &texture);
    stbi_image_free(image_data);

    if (FAILED(hr)) return nullptr;

    ID3D11ShaderResourceView* srv = nullptr;
    render->detail_ptr->device->CreateShaderResourceView(texture, nullptr, &srv);
    texture->Release();

    return srv;
}
static ID3D11ShaderResourceView* load_texture_from_url(std::uint64_t userId) {
    static std::map<std::uint64_t, ID3D11ShaderResourceView*> texture_cache;
    static std::set<std::uint64_t> loading_textures;
    static std::mutex texture_mtx;

    std::lock_guard<std::mutex> lock(texture_mtx);
    if (texture_cache.count(userId)) return texture_cache[userId];
    if (loading_textures.count(userId)) return nullptr;

    loading_textures.insert(userId);

    std::thread([userId]() {
        char url[256];
        sprintf_s(url, "https://www.roblox.com/headshot-thumbnail/image?userId=%llu&width=150&height=150&format=png", userId);

        auto data = download_url(url);
        if (!data.empty()) {
            auto srv = create_texture_from_memory(data);
            if (srv) {
                std::lock_guard<std::mutex> lock(texture_mtx);
                texture_cache[userId] = srv;
            }
        }

        std::lock_guard<std::mutex> lock(texture_mtx);
        loading_textures.erase(userId);
    }).detach();

    return nullptr;
}

void c_gui::render()
{
	var->gui.menu_alpha = ImClamp(var->gui.menu_alpha + (gui->fixed_speed(8.f) * (var->gui.menu_opened ? 1.f : -1.f)), 0.f, 1.f);

	if (var->gui.menu_alpha <= 0.01f)
		return;

	gui->set_next_window_pos(ImVec2(GetIO().DisplaySize.x / 2 - var->window.width / 2, 20));
	gui->set_next_window_size(ImVec2(var->window.width, elements->section.size.y + var->window.spacing.y * 2 - 1));
	gui->push_style_var(ImGuiStyleVar_Alpha, var->gui.menu_alpha);
	gui->begin("Game", nullptr, var->window.main_flags);
	{
		const ImVec2 pos = GetWindowPos();
		const ImVec2 size = GetWindowSize();
		ImDrawList* draw_list = GetWindowDrawList();
		ImGuiStyle* style = &GetStyle();

		{
			style->WindowPadding = var->window.padding;
			style->PopupBorderSize = var->window.border_size;
			style->WindowBorderSize = var->window.border_size;
			style->ItemSpacing = var->window.spacing;
			style->WindowShadowSize = var->window.window_glow ? var->window.shadow_size : 0.f;
			style->ScrollbarSize = var->window.scrollbar_size;
			style->Colors[ImGuiCol_WindowShadow] = { clr->accent.Value.x, clr->accent.Value.y, clr->accent.Value.z, var->window.shadow_alpha };
		}

		{
			draw->rect(GetBackgroundDrawList(), pos - ImVec2(1, 1), pos + size + ImVec2(1, 1), draw->get_clr({ 0, 0, 0, 0.5f }));
			draw->rect_filled(draw_list, pos, pos + size, draw->get_clr(clr->window.background_one));
			draw->line(draw_list, pos + ImVec2(1, 1), pos + ImVec2(size.x - 1, 1), draw->get_clr(clr->accent), 1);
			draw->line(draw_list, pos + ImVec2(1, 2), pos + ImVec2(size.x - 1, 2), draw->get_clr(clr->accent, 0.4f), 1);
			draw->rect(draw_list, pos, pos + size, draw->get_clr(clr->window.stroke));
		}

		{
			gui->set_cursor_pos(style->ItemSpacing);
			gui->begin_group();
			{
				for (int i = 0; i < IM_ARRAYSIZE(var->gui.current_section); i++)
					gui->section(var->gui.section_icons[i], &var->gui.current_section[i]);
			}
			gui->end_group();
		}

		{
			static int section = 0;
			static int visuals_subtab = 0;
			static int style_subtab = 0;

			if (var->gui.current_section[0])
			{
				gui->set_next_window_size(ImVec2(600, 650));
				gui->begin("Home", nullptr, var->window.flags);
				{
					draw->window_decorations();

					gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar));
					gui->begin_group();
					{
						gui->sub_section("Aimbot", 0, section, 4);
						gui->sub_section("Players", 1, section, 4);
						gui->sub_section("Visuals", 2, section, 4);
						gui->sub_section("Misc", 3, section, 4);
					}
					gui->end_group();

					gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + elements->section.height - 1));
					gui->begin_content();
					{
						if (section == 0)
						{
							static const char* method_types[] = { "Mouse", "Camera", "Teleport" };
							static const char* hitbox_labels[] = { "Head", "Torso", "Left Arm", "Right Arm", "Left Leg", "Right Leg" };
							static const char* hitscan_types[] = { "None", "Static", "Dynamic" };
							static const char* smoothing_types[] = { "Linear", "Exponential", "Cubic" };
							static const char* aiming_checks[] = { "Team", "Visible", "Forcefield", "Alive", "Tool", "Ragdoll", "Godded", "Melee" };
							static const char* trigger_modes[] = { "Always", "On Key", "Toggle" };
							static const char* trigger_hitboxes[] = { "Head", "Torso", "Left Arm", "Right Arm" };

							gui->begin_group();
							{
								gui->begin_child("Aiming", 2, 2, ImVec2(0, 0));
								{
									gui->checkbox("Enabled", &globals::aimbot::enabled, &globals::aimbot::keybind, &globals::aimbot::keybind_mode);
									gui->dropdown("Method", &globals::aimbot::method, method_types, IM_ARRAYSIZE(method_types));
									gui->multi_dropdown("Hitboxes", globals::aimbot::hitboxes, hitbox_labels, IM_ARRAYSIZE(hitbox_labels));
									gui->dropdown("Hitscan", &globals::aimbot::hitscan_type, hitscan_types, IM_ARRAYSIZE(hitscan_types));

									gui->slider_float("FOV", &globals::aimbot::fov, 0.0f, 100.0f, false, "%.1f%%/100.0%%");
									gui->slider_int("Radius", &globals::aimbot::radius, 0, 500, false, "%d/500");

									gui->checkbox("Deadzone", &globals::aimbot::deadzone);
									gui->slider_float("Deadzone Value", &globals::aimbot::deadzone_value, 0.0f, 25.0f, false, "%.1f/25.0");

									gui->checkbox("Smoothing", &globals::aimbot::smoothing);
									gui->slider_float("Smoothing X", &globals::aimbot::smoothing_values[0], 0.0f, 100.0f, false, "%.1f/100.0");
									gui->slider_float("Smoothing Y", &globals::aimbot::smoothing_values[1], 0.0f, 100.0f, false, "%.1f/100.0");

									gui->checkbox("Wallcheck", &globals::aimbot::wallcheck);
									gui->checkbox("Auto Wall", &globals::aimbot::auto_wall);

									static int smoothing_type = 0;
									gui->dropdown("Smoothing Type", &smoothing_type, smoothing_types, IM_ARRAYSIZE(smoothing_types));

									gui->checkbox("Stick to target", &globals::aimbot::root_on_void);
									gui->multi_dropdown("Aiming checks", globals::aimbot::aiming_checks, aiming_checks, IM_ARRAYSIZE(aiming_checks));
								}
								gui->end_child();

								gui->begin_child("Trigger", 2, 4, ImVec2(0, 0));
								{
									gui->checkbox("Enabled", &globals::aimbot::trigger::enabled, &globals::aimbot::trigger::keybind, &globals::aimbot::trigger::keybind_mode);
									gui->dropdown("Mode", &globals::aimbot::trigger::keybind_mode, trigger_modes, IM_ARRAYSIZE(trigger_modes));
									gui->slider_float("Delay", &globals::aimbot::trigger::delay, 0.0f, 1000.0f, false, "%.1f/1000.0ms");
									gui->slider_float("Interval", &globals::aimbot::trigger::interval, 0.0f, 1000.0f, false, "%.1f/1000.0ms");

									static const char* trigger_hitboxes[] = { "Head", "Torso", "Arms", "Legs" };
									gui->multi_dropdown("Hitboxes", globals::aimbot::trigger::hitboxes, trigger_hitboxes, IM_ARRAYSIZE(trigger_hitboxes));
								}
								gui->end_child();

								gui->begin_child("Misc", 2, 4, ImVec2(0, 0));
								{
									gui->checkbox("Recoil Reduction", &globals::misc::recoil_reduction);
									gui->slider_float("Recoil", &globals::misc::recoil_reduction_value, 0.0f, 100.0f, false, "%.1f%%/100.0%%");
									gui->checkbox("Map Debug", &globals::visuals::map_debug);
									if (ImGui::Button("Parse Map", ImVec2(-1, 0))) {
										cheats::misc::map_parser_scan();
									}
								}
								gui->end_child();
							}
							gui->end_group();

							gui->sameline();

							gui->begin_group();
							{
								gui->begin_child("Prediction", 0, 2, ImVec2(0, 0));
								{
							
									if (gui->checkbox("Enabled", &globals::aimbot::prediction::enabled)) {
										notify->add("TESTING", "testingg", 5.0f, notify_pos_t::bottom_right);
									}
								
									gui->checkbox("Velocity prediction", &globals::aimbot::prediction::velocity_prediction);
									gui->slider_float("Horizontal Prediction", &globals::aimbot::prediction::x_multiplier, 0.0f, 20.0f, false, "%.1f/20.0");
									gui->slider_float("Vertical Prediction", &globals::aimbot::prediction::y_multiplier, 0.0f, 20.0f, false, "%.1f/20.0");
									gui->checkbox("Bullet speed", &globals::aimbot::prediction::bullet_speed);
									gui->slider_float("Speed", &globals::aimbot::prediction::bullet_speed_value, 100.0f, 5000.0f, false, "%.1f/5000.0");
									gui->checkbox("Gravity compensation", &globals::aimbot::prediction::bullet_gravity);
									gui->slider_float("Gravity", &globals::aimbot::prediction::bullet_gravity_value, 0.0f, 196.2f, false, "%.1f/196.2");
								}
								gui->end_child();

								gui->begin_child("Target Strafe", 0, 2, ImVec2(0, 0));
								{
									gui->checkbox("Enabled", &globals::aimbot::target_strafe::enabled);
									gui->slider_float("Speed", &globals::aimbot::target_strafe::speed, 0.0f, 100.0f, false, "%.1f/100.0");
									gui->slider_float("Radius", &globals::aimbot::target_strafe::radius, 0.0f, 100.0f, false, "%.1f/100.0");
									gui->slider_float("Height", &globals::aimbot::target_strafe::height, -100.0f, 100.0f, false, "%.1f/100.0");
								}
								gui->end_child();
							}
							gui->end_group();
						}

						if (section == 1)
						{
							gui->begin_group();
							{
								ImGuiID players_id = GetCurrentWindow()->GetID("Players Options");
								visuals_subtab = gui->get_child_subtab(players_id);

								gui->begin_multi_subtab(
									"Players Options",
									2,
									2,
									3,
									ImVec2(0, 310),
									{ "Enemies", "Friendlies", "Client" }
								);

								auto render_player_tab = [&](globals::visuals::player_settings_t& settings) {
									gui->checkbox("Enabled", &settings.enabled);
									gui->checkbox("Wallcheck", &globals::visuals::options::wallcheck);

									float name_col[4]; draw->ImU32ToFloat(settings.colors.name, name_col);
									float name_glow_col[4]; draw->ImU32ToFloat(settings.colors.name_glow, name_glow_col);
									gui->checkbox("Name", &settings.name, { name_col, name_glow_col });
									settings.colors.name = draw->FloatToImU32(name_col);
									settings.colors.name_glow = draw->FloatToImU32(name_glow_col);

									if (settings.name)
									{
										gui->slider_float("Thickness", &globals::visuals::options::text_outline_thickness, 1.0f, 5.0f, false, "%.1fpx");

										if (settings.visibility_colors)
										{
											float name_vis_col[4]; draw->ImU32ToFloat(settings.colors.name_vis, name_vis_col);
											float name_invis_col[4]; draw->ImU32ToFloat(settings.colors.name_invis, name_invis_col);

											if (gui->checkbox("Name Visibility Colors", &settings.visibility_colors, { name_vis_col, name_invis_col })) {}
											settings.colors.name_vis = draw->FloatToImU32(name_vis_col);
											settings.colors.name_invis = draw->FloatToImU32(name_invis_col);
										}
										else
										{
											gui->checkbox("Name Visibility Colors", &settings.visibility_colors);
										}
									}

									float box_col[4]; draw->ImU32ToFloat(settings.colors.box, box_col);
									float box_glow_col[4]; draw->ImU32ToFloat(settings.colors.box_glow, box_glow_col);
									gui->checkbox("Bounding Box", &settings.bounding_box, { box_col, box_glow_col });
									settings.colors.box = draw->FloatToImU32(box_col);
									settings.colors.box_glow = draw->FloatToImU32(box_glow_col);

									if (settings.bounding_box)
									{
										gui->slider_float("Thickness", &globals::visuals::options::outline_thickness, 1.0f, 5.0f, false, "%.1fpx");

										if (settings.visibility_colors)
										{
											float box_vis_col[4]; draw->ImU32ToFloat(settings.colors.box_vis, box_vis_col);
											float box_invis_col[4]; draw->ImU32ToFloat(settings.colors.box_invis, box_invis_col);

											if (gui->checkbox("Box Visibility Colors", &settings.visibility_colors, { box_vis_col, box_invis_col })) {}
											settings.colors.box_vis = draw->FloatToImU32(box_vis_col);
											settings.colors.box_invis = draw->FloatToImU32(box_invis_col);
										}
										else
										{
											gui->checkbox("Box Visibility Colors", &settings.visibility_colors);
										}

										const char* box_types[] = { "Dynamic", "Static" };
										gui->dropdown("Box Calculation", &settings.bounding_box_type, box_types, IM_ARRAYSIZE(box_types));

										const char* box_styles[] = { "Regular", "Corner", "3D" };
										gui->dropdown("Box Style", &settings.bounding_box_style, box_styles, IM_ARRAYSIZE(box_styles));

										if (settings.bounding_box_style == 1)
										{
											gui->slider_float("Corner Length", &settings.corner_box_length, 0.1f, 1.0f, false, "%.2f");
										}
									}


									float box_fill_col1[4]; draw->ImU32ToFloat(settings.colors.box_fill[0], box_fill_col1);
									float box_fill_col2[4]; draw->ImU32ToFloat(settings.colors.box_fill[1], box_fill_col2);
									gui->checkbox("Box Fill", &settings.box_fill, { box_fill_col1, box_fill_col2 });
									settings.colors.box_fill[0] = draw->FloatToImU32(box_fill_col1);
									settings.colors.box_fill[1] = draw->FloatToImU32(box_fill_col2);

									gui->checkbox("Box Fill Gradient", &settings.box_fill_gradient);
									gui->checkbox("Box Fill Gradient Spin", &settings.box_fill_gradient_spin);
									if (settings.box_fill_gradient_spin)
										gui->slider_float("Gradient Speed", &settings.box_fill_gradient_speed, 0.1f, 10.0f, false, "%.1fx");

									const char* gradient_styles[] = { "Side-to-Side", "Bottom-to-Top", "Spin" };
									gui->dropdown("Gradient Style", &settings.box_fill_gradient_style, gradient_styles, IM_ARRAYSIZE(gradient_styles));

									float hb_col[4]; draw->ImU32ToFloat(settings.colors.healthbar, hb_col);
									float hb_glow_col[4]; draw->ImU32ToFloat(settings.colors.healthbar_glow, hb_glow_col);
									gui->checkbox("Health Bar", &settings.healthbar, { hb_col, hb_glow_col });
									settings.colors.healthbar = draw->FloatToImU32(hb_col);
									settings.colors.healthbar_glow = draw->FloatToImU32(hb_glow_col);

									float hbt_col[4]; draw->ImU32ToFloat(settings.colors.healthbar_text, hbt_col);
									gui->checkbox("Health Value", &settings.healthbar_text, hbt_col);
									settings.colors.healthbar_text = draw->FloatToImU32(hbt_col);

									float hg_col1[4]; draw->ImU32ToFloat(settings.colors.healthbar_gradient[0], hg_col1);
									float hg_col2[4]; draw->ImU32ToFloat(settings.colors.healthbar_gradient[1], hg_col2);
									float hg_col3[4]; draw->ImU32ToFloat(settings.colors.healthbar_gradient[2], hg_col3);

									if (gui->checkbox("Health Gradient", &settings.healthbar_gradient, { hg_col1, hg_col2, hg_col3 }))
									{
									}

									settings.colors.healthbar_gradient[0] = draw->FloatToImU32(hg_col1);
									settings.colors.healthbar_gradient[1] = draw->FloatToImU32(hg_col2);
									settings.colors.healthbar_gradient[2] = draw->FloatToImU32(hg_col3);

									gui->checkbox("Health Lerp", &settings.healthbar_lerp);

									float arrows_col[4]; draw->ImU32ToFloat(settings.colors.arrows, arrows_col);
									float arrows_glow_col[4]; draw->ImU32ToFloat(settings.colors.arrows_glow, arrows_glow_col);
									gui->checkbox("Arrows", &settings.arrows, { arrows_col, arrows_glow_col });
									settings.colors.arrows = draw->FloatToImU32(arrows_col);
									settings.colors.arrows_glow = draw->FloatToImU32(arrows_glow_col);

									gui->slider_float("Arrows Size", &settings.arrows_size, 1, 100, "%.0fpx/100px");
									gui->slider_float("Arrows Position", &settings.arrows_position, 1, 100, "%.1f%%/100.0%%");

									const char* arrow_info_labels[] = { "Name", "Distance" };
									gui->multi_dropdown("Arrow Info", settings.arrows_info, arrow_info_labels, IM_ARRAYSIZE(arrow_info_labels));

									gui->slider_float("Arrows Max Distance", &settings.arrows_max_dist, 0, 5000, "%.0fst/5000st");

									const char* arrow_types[] = { "Normal", "Flat" };
									gui->dropdown("Arrow Type", &settings.arrow_type, arrow_types, IM_ARRAYSIZE(arrow_types));

									float head_dot_col[4]; draw->ImU32ToFloat(settings.colors.head_dot, head_dot_col);
									float head_dot_glow_col[4]; draw->ImU32ToFloat(settings.colors.head_dot_glow, head_dot_glow_col);
									gui->checkbox("Head Dot", &settings.head_dot, { head_dot_col, head_dot_glow_col });
									settings.colors.head_dot = draw->FloatToImU32(head_dot_col);
									settings.colors.head_dot_glow = draw->FloatToImU32(head_dot_glow_col);

									float tool_col[4]; draw->ImU32ToFloat(settings.colors.tool, tool_col);
									gui->checkbox("Tool", &settings.tool, tool_col);
									settings.colors.tool = draw->FloatToImU32(tool_col);

									float dist_col[4]; draw->ImU32ToFloat(settings.colors.distance, dist_col);
									float dist_glow_col[4]; draw->ImU32ToFloat(settings.colors.distance_glow, dist_glow_col);
									gui->checkbox("Distance", &settings.distance, { dist_col, dist_glow_col });
									settings.colors.distance = draw->FloatToImU32(dist_col);
									settings.colors.distance_glow = draw->FloatToImU32(dist_glow_col);

									if (settings.distance)
									{
									}

									gui->checkbox("State Flags", &settings.state_flags);
									gui->checkbox("Rig Flags", &settings.rig_flags);

									float chams_col[4]; draw->ImU32ToFloat(settings.colors.chams, chams_col);
									float chams_fresnel_col[4]; draw->ImU32ToFloat(settings.colors.chams_fresnel, chams_fresnel_col);
									gui->checkbox("Chams", &settings.chams, { chams_col, chams_fresnel_col });
									settings.colors.chams = draw->FloatToImU32(chams_col);
									settings.colors.chams_fresnel = draw->FloatToImU32(chams_fresnel_col);

									if (settings.chams)
									{
										gui->slider_float("Thickness", &globals::visuals::options::chams_outline_thickness, 1.0f, 5.0f, false, "%.1fpx");
										gui->checkbox("Health Based", &settings.chams_health_based);

										float chams_glow_col[4]; draw->ImU32ToFloat(settings.colors.chams_glow, chams_glow_col);
										gui->checkbox("Chams Glow", &settings.chams_glow, chams_glow_col);
										settings.colors.chams_glow = draw->FloatToImU32(chams_glow_col);

										if (settings.chams_glow)
										{
											gui->slider_float("Glow Intensity", &settings.chams_glow_intensity, 1.0f, 100.0f, false, "%.0f%%");
										}

										const char* chams_textures[] = { "Solid", "Bubble", "ForceField", "Neon", "Glass", "Wood", "Slate" };
										gui->dropdown("Chams Texture", &settings.chams_texture, chams_textures, IM_ARRAYSIZE(chams_textures));

										float flash_col[4]; draw->ImU32ToFloat(settings.colors.chams_flash, flash_col);
										gui->checkbox("Chams Flash", &settings.chams_flash, flash_col);
										settings.colors.chams_flash = draw->FloatToImU32(flash_col);

										if (settings.chams_flash)
										{
											gui->slider_float("Flash Speed", &settings.chams_flash_speed, 0.1f, 10.0f, false, "%.1fx");
										}

										gui->checkbox("Chams Fade", &settings.chams_fade);
										if (settings.chams_fade)
											gui->slider_float("Fade Speed", &settings.chams_fade_speed, 0.1f, 10.0f, false, "%.1fx");
									}

									float tracers_col[4]; draw->ImU32ToFloat(settings.colors.tracers, tracers_col);
									gui->checkbox("Tracers", &settings.tracers, tracers_col);
									settings.colors.tracers = draw->FloatToImU32(tracers_col);

									if (settings.tracers)
									{
										gui->slider_float("Tracer Thickness", &settings.tracers_thickness, 1.0f, 5.0f, false, "%.1fpx");
										const char* tracer_origins[] = { "Bottom", "Top", "Center", "Cursor" };
										gui->dropdown("Tracer Origin", &settings.tracers_origin, tracer_origins, IM_ARRAYSIZE(tracer_origins));
									}

									float mesh_chams_col[4]; draw->ImU32ToFloat(settings.colors.mesh_chams, mesh_chams_col);
									float mesh_chams_glow_col[4]; draw->ImU32ToFloat(settings.colors.mesh_chams_glow, mesh_chams_glow_col);
									gui->checkbox("Mesh Chams", &settings.mesh_chams, { mesh_chams_col, mesh_chams_glow_col });
									settings.colors.mesh_chams = draw->FloatToImU32(mesh_chams_col);
									settings.colors.mesh_chams_glow = draw->FloatToImU32(mesh_chams_glow_col);

									if (settings.mesh_chams)
									{
									}

									float skeleton_col[4]; draw->ImU32ToFloat(settings.colors.skeleton, skeleton_col);
									float skeleton_glow_col[4]; draw->ImU32ToFloat(settings.colors.skeleton_glow, skeleton_glow_col);
									gui->checkbox("Skeleton", &settings.skeleton, { skeleton_col, skeleton_glow_col });
									settings.colors.skeleton = draw->FloatToImU32(skeleton_col);
									settings.colors.skeleton_glow = draw->FloatToImU32(skeleton_glow_col);

									if (settings.skeleton)
									{
										gui->slider_float("Thickness", &globals::visuals::options::skeleton_outline_thickness, 1.0f, 5.0f, false, "%.1fpx");
									}

									float sound_col[4]; draw->ImU32ToFloat(settings.colors.sound_esp, sound_col);
									float sound_glow_col[4]; draw->ImU32ToFloat(settings.colors.sound_esp_glow, sound_glow_col);
									gui->checkbox("Sound ESP", &settings.sound_esp, { sound_col, sound_glow_col });
									settings.colors.sound_esp = draw->FloatToImU32(sound_col);
									settings.colors.sound_esp_glow = draw->FloatToImU32(sound_glow_col);

									if (settings.sound_esp)
									{
										gui->slider_float("Sound Radius", &settings.sound_esp_radius, 1.0f, 20.0f, false, "%.1f studs");
										gui->slider_float("Sound Speed", &settings.sound_esp_speed, 0.1f, 5.0f, false, "%.1fx");
									}

									float footprint_col[4]; draw->ImU32ToFloat(settings.colors.footprints, footprint_col);
									float footprint_glow_col[4]; draw->ImU32ToFloat(settings.colors.footprints_glow, footprint_glow_col);
									gui->checkbox("Footprints", &settings.footprints, { footprint_col, footprint_glow_col });
									settings.colors.footprints = draw->FloatToImU32(footprint_col);
									settings.colors.footprints_glow = draw->FloatToImU32(footprint_glow_col);

									if (settings.footprints)
									{
										gui->checkbox("Footprints Glow", &settings.footprints_glow);
										gui->slider_float("Footprint Scale", &settings.footprints_radius, 0.1f, 5.0f, false, "%.1fx");
										gui->slider_float("Footprint Interval", &settings.footprints_speed, 0.1f, 5.0f, false, "%.1fx");
									}

									float trail_col[4]; draw->ImU32ToFloat(settings.colors.movement_trails, trail_col);
									float trail_glow_col[4]; draw->ImU32ToFloat(settings.colors.movement_trails_glow, trail_glow_col);
									gui->checkbox("Movement Trails", &settings.movement_trails, { trail_col, trail_glow_col });
									settings.colors.movement_trails = draw->FloatToImU32(trail_col);
									settings.colors.movement_trails_glow = draw->FloatToImU32(trail_glow_col);

									if (settings.movement_trails)
									{
										gui->slider_float("Trail Thickness", &settings.movement_trails_thickness, 0.1f, 10.0f, false, "%.1fpx");
										gui->slider_float("Trail Length", &settings.movement_trails_length, 0.1f, 10.0f, false, "%.1fs");
									}

									float angle_col[4]; draw->ImU32ToFloat(settings.colors.view_angle_lines, angle_col);
									float angle_glow_col[4]; draw->ImU32ToFloat(settings.colors.view_angle_lines_glow, angle_glow_col);
									gui->checkbox("View Angle Lines", &settings.view_angle_lines, { angle_col, angle_glow_col });
									settings.colors.view_angle_lines = draw->FloatToImU32(angle_col);
									settings.colors.view_angle_lines_glow = draw->FloatToImU32(angle_glow_col);

									if (settings.view_angle_lines)
									{
										gui->slider_float("Line Thickness", &settings.view_angle_lines_thickness, 0.1f, 5.0f, false, "%.1fpx");
										gui->slider_float("Line Length", &settings.view_angle_lines_length, 1.0f, 100.0f, false, "%.0f studs");
									}
									};

								if (visuals_subtab == 0) render_player_tab(globals::visuals::enemies);
								if (visuals_subtab == 1) render_player_tab(globals::visuals::friendlies);
								if (visuals_subtab == 2) render_player_tab(globals::visuals::local_player);

								gui->end_child();

								gui->begin_child("Other", 2, 2, ImVec2(0, 270));
								{
									float c_chams_col[4]; draw->ImU32ToFloat(globals::visuals::client::colors::corpse_chams, c_chams_col);
									gui->checkbox("Corpse Chams", &globals::visuals::client::corpse_chams, c_chams_col);
									globals::visuals::client::colors::corpse_chams = draw->FloatToImU32(c_chams_col);

									if (globals::visuals::client::corpse_chams)
									{
									}

									float c_names_col[4]; draw->ImU32ToFloat(globals::visuals::client::colors::corpse_names, c_names_col);
									gui->checkbox("Corpse Names", &globals::visuals::client::corpse_names, c_names_col);
									globals::visuals::client::colors::corpse_names = draw->FloatToImU32(c_names_col);

									if (globals::visuals::client::corpse_names)
									{
									}

									const char* corpse_types[] = { "Corpse" };
									gui->dropdown("Corpse", &globals::visuals::client::corpse_type, corpse_types, IM_ARRAYSIZE(corpse_types));

									gui->checkbox("Corpse Shift", &globals::visuals::client::corpse_shift);
									gui->slider_float("Corpse Max Distance", &globals::visuals::client::corpse_max_dist, 0, 5000, "%.0fst/5000st");

									float e_h_col[4]; draw->ImU32ToFloat(globals::visuals::client::colors::enemy_highlight, e_h_col);
									gui->checkbox("Enemy Highlight", &globals::visuals::client::enemy_highlight, e_h_col);
									globals::visuals::client::colors::enemy_highlight = draw->FloatToImU32(e_h_col);

									if (globals::visuals::client::enemy_highlight)
									{
									}

									float f_h_col[4]; draw->ImU32ToFloat(globals::visuals::client::colors::friend_highlight, f_h_col);
									gui->checkbox("Friend Highlight", &globals::visuals::client::friend_highlight, f_h_col);
									globals::visuals::client::colors::friend_highlight = draw->FloatToImU32(f_h_col);

									if (globals::visuals::client::friend_highlight)
									{
									}

									float cross_col[4]; draw->ImU32ToFloat(globals::visuals::crosshair::color, cross_col);
									gui->checkbox("Crosshair", &globals::visuals::crosshair::enabled, cross_col);
									globals::visuals::crosshair::color = draw->FloatToImU32(cross_col);

									if (globals::visuals::crosshair::enabled)
									{
										gui->checkbox("Crosshair Rainbow", &globals::visuals::crosshair::rainbow);
										gui->checkbox("Crosshair Spin", &globals::visuals::crosshair::spin);
										if (globals::visuals::crosshair::spin)
											gui->slider_float("Spin Speed", &globals::visuals::crosshair::spin_speed, 0.1f, 100.0f);
										gui->checkbox("Crosshair Pulse", &globals::visuals::crosshair::pulse);
										if (globals::visuals::crosshair::pulse)
											gui->slider_float("Pulse Speed", &globals::visuals::crosshair::pulse_speed, 0.1f, 100.0f);
										const char* cross_origins[] = { "Center", "Mouse", "Cursor to Target" };
										gui->dropdown("Crosshair Origin", &globals::visuals::crosshair::origin, cross_origins, IM_ARRAYSIZE(cross_origins));
										gui->checkbox("Crosshair Lerp", &globals::visuals::crosshair::lerp);
										if (globals::visuals::crosshair::lerp)
											gui->slider_float("Lerp Speed", &globals::visuals::crosshair::lerp_speed, 1.0f, 50.0f);
										gui->slider_float("Crosshair Size", &globals::visuals::crosshair::size, 1.0f, 50.0f);
										gui->slider_float("Crosshair Thickness", &globals::visuals::crosshair::thickness, 1.0f, 10.0f);
										gui->slider_float("Crosshair Gap", &globals::visuals::crosshair::gap, 0.0f, 50.0f);
										float dot_col[4]; draw->ImU32ToFloat(globals::visuals::crosshair::dot_color, dot_col);
										gui->checkbox("Crosshair Dot", &globals::visuals::crosshair::dot, dot_col);
										globals::visuals::crosshair::dot_color = draw->FloatToImU32(dot_col);

										if (globals::visuals::crosshair::dot)
										{
											gui->slider_float("Dot Size", &globals::visuals::crosshair::dot_size, 1.0f, 10.0f);
											gui->checkbox("Dot Outline", &globals::visuals::crosshair::dot_outline);
										}
										gui->checkbox("Crosshair Outlines", &globals::visuals::crosshair::outline);
										if (globals::visuals::crosshair::outline)
											gui->slider_float("Outline Thickness", &globals::visuals::crosshair::outline_thickness, 1.0f, 5.0f);
										gui->checkbox("T-Style", &globals::visuals::crosshair::t_style);
										gui->checkbox("Sniper Mode", &globals::visuals::crosshair::sniper_crosshair);
									}

									float fov_col[4]; draw->ImU32ToFloat(globals::visuals::fov_circle::color, fov_col);
									gui->checkbox("FOV Circle", &globals::visuals::fov_circle::enabled, fov_col);
									globals::visuals::fov_circle::color = draw->FloatToImU32(fov_col);

									if (globals::visuals::fov_circle::enabled)
									{
										gui->checkbox("FOV Rainbow", &globals::visuals::fov_circle::rainbow);
										
										float fill_col[4]; draw->ImU32ToFloat(globals::visuals::fov_circle::fill_color, fill_col);
										gui->checkbox("FOV Filled", &globals::visuals::fov_circle::filled, fill_col);
										globals::visuals::fov_circle::fill_color = draw->FloatToImU32(fill_col);

										gui->slider_float("FOV Thickness", &globals::visuals::fov_circle::thickness, 1.0f, 10.0f);
										gui->slider_int("FOV Sides", &globals::visuals::fov_circle::num_sides, 3, 100);
										const char* fov_origins[] = { "Center", "Mouse", "Cursor to Target" };
										gui->dropdown("FOV Origin", &globals::visuals::fov_circle::origin, fov_origins, IM_ARRAYSIZE(fov_origins));
									}
								}
								gui->end_child();
							}
							gui->end_group();

							gui->sameline();

							gui->begin_group();
							{
								gui->begin_child("Options", 0, 0);
								{
									gui->checkbox("Visible Check", &globals::visuals::options::visible_check);
									gui->checkbox("Forcefield Check", &globals::visuals::options::forcefield_check);
									gui->checkbox("Alive Check", &globals::visuals::options::alive_check);
									gui->checkbox("Tool Check", &globals::visuals::options::tool_check);
									gui->checkbox("Ragdoll Check", &globals::visuals::options::ragdoll_check);
									gui->checkbox("Godded Check", &globals::visuals::options::godded_check);
									gui->checkbox("Sort By Status", &globals::visuals::options::sort_by_status);

									const char* outline_labels[] = { "Name", "Box", "Health", "Arrows", "Head Dot", "Distance", "Skeleton", "Fresnel", "Crosshair" };
									gui->multi_dropdown("Render Outlines", globals::visuals::options::render_outlines, outline_labels, IM_ARRAYSIZE(outline_labels));

									const char* global_outline_types[] = { "None", "Outline", "Shadow" };
									gui->dropdown("Global Outline Type", &globals::visuals::options::global_outline_type, global_outline_types, IM_ARRAYSIZE(global_outline_types));

									gui->slider_float("Max Distance", &globals::visuals::options::max_distance, 0.f, 10000.f);
									gui->slider_float("Max Size", &globals::visuals::options::max_size, 0.f, 100.f);

									gui->checkbox("Box Inline", &globals::visuals::options::box_inline);

									gui->checkbox("Bar Fold", &globals::visuals::options::bar_fold);
									gui->slider_float("Bar Size", &globals::visuals::options::bar_size, 0.1f, 5.0f);

									gui->checkbox("Value Follow", &globals::visuals::options::value_follow);
									gui->checkbox("Ignore Full", &globals::visuals::options::ignore_full);

									const char* name_transforms[] = { "None", "Uppercase", "Lowercase" };
									gui->dropdown("Name Transform", &globals::visuals::options::name_transform, name_transforms, IM_ARRAYSIZE(name_transforms));

									const char* font_types[] = { "Tahoma", "Visitor" };
									gui->dropdown("ESP Font", &globals::visuals::options::font_type, font_types, IM_ARRAYSIZE(font_types));

									gui->checkbox("Use Display Name", &globals::visuals::options::use_display_name);
									gui->checkbox("Combined Name", &globals::visuals::options::combined_name);
									const char* head_types[] = { "Dot", "Hexagon" };
									gui->dropdown("Head Type", &globals::visuals::options::head_type, head_types, IM_ARRAYSIZE(head_types));
									
									if (globals::visuals::options::head_type == 1)
									{
										const char* head_modes[] = { "Static", "Bounding" };
										gui->dropdown("Head Mode", &globals::visuals::options::head_mode, head_modes, IM_ARRAYSIZE(head_modes));

										if (globals::visuals::options::head_mode == 0)
											gui->slider_float("Head Size", &globals::visuals::options::head_static_size, 0.1f, 10.0f, false, "%.1f");
										else if (globals::visuals::options::head_mode == 1)
											gui->slider_float("Size Limit", &globals::visuals::options::head_size_limit, 1.0f, 10.0f, false, "%.1f");
									}

									gui->checkbox("Circular Head Dot", &globals::visuals::options::circular_head_dot);
									gui->checkbox("Show None", &globals::visuals::options::show_none);
									gui->checkbox("Static On Death", &globals::visuals::options::static_on_death);
									gui->checkbox("Static On Void", &globals::visuals::options::static_on_void);

									float ch_col[4]; draw->ImU32ToFloat(globals::visuals::colors::chams_hit_impact, ch_col);
									gui->checkbox("Chams Hit Impact", &globals::visuals::options::chams_hit_impact, ch_col);
									globals::visuals::colors::chams_hit_impact = draw->FloatToImU32(ch_col);

									if (globals::visuals::options::chams_hit_impact)
									{
									}

									const char* arrow_modes[] = { "Screen Center", "Mouse Position" };
									gui->dropdown("Arrows Mode", &globals::visuals::options::offscreen_arrows_mode, arrow_modes, IM_ARRAYSIZE(arrow_modes));
									gui->slider_float("Arrows Radius", &globals::visuals::options::offscreen_arrows_radius, 50.f, 500.f);
									gui->slider_float("Arrows Size", &globals::visuals::options::offscreen_arrows_size, 5.f, 50.f);
									gui->checkbox("Arrows Glow", &globals::visuals::options::offscreen_arrows_glow);
								}
								gui->end_child();
							}
							gui->end_group();
						}

						if (section == 2)
						{


							gui->begin_group();
							{
								gui->begin_child("Graphic", 2, 1, ImVec2(0, 310));
								{

									
								gui->checkbox("Terrain Features", &globals::visuals::terrain::enabled);
									
										gui->slider_float("Grass Length", &globals::visuals::terrain::grass_length, 0.f, 1.f);
										gui->label_color_edit("Water Color", globals::visuals::terrain::water_color);
										gui->slider_float("Water Reflectance", &globals::visuals::terrain::water_reflectance, 0.f, 1.f);
										gui->slider_float("Water Transparency", &globals::visuals::terrain::water_transparency, 0.f, 1.f);
										gui->slider_float("Water Wave Size", &globals::visuals::terrain::water_wave_size, 0.f, 1.f);
										gui->slider_float("Water Wave Speed", &globals::visuals::terrain::water_wave_speed, 0.f, 1.f);
									
								}
								gui->end_child();

								gui->begin_child("Misc", 2, 1, ImVec2(0, 270));
								{
									gui->checkbox("Master Switch", &globals::visuals::masterswitch);

									gui->checkbox("Show Visuals", &globals::visuals::show_visuals);

									gui->warning_checkbox("Fullbright", &globals::visuals::fullbright, &globals::visuals::fullbright_key, &globals::visuals::fullbright_mode);

									gui->warning_checkbox("No Fog", &globals::visuals::no_fog, &globals::visuals::no_fog_key, &globals::visuals::no_fog_mode);

									gui->checkbox("Third Person", &globals::visuals::third_person);

									if (globals::visuals::third_person)
									{
										gui->slider_float("Min Zoom", &globals::visuals::min_zoom, 0.5f, 50.f, "%.1f");
										gui->slider_float("Max Zoom", &globals::visuals::max_zoom, 50.f, 1000.f, "%.0f");
									}
									gui->checkbox("Auto Jump", &globals::visuals::auto_jump);

									gui->checkbox("Auto Rotate", &globals::visuals::auto_rotate);

									gui->begin_child("ViewModel", 0, 0, ImVec2(0, 0));
									{
										gui->checkbox("Enabled", &globals::visuals::viewmodel::enabled);
										gui->slider_float("FOV", &globals::visuals::viewmodel::fov, 30.f, 120.f);
										gui->slider_float("Offset X", &globals::visuals::viewmodel::offset[0], -5.f, 5.f);
										gui->slider_float("Offset Y", &globals::visuals::viewmodel::offset[1], -5.f, 5.f);
										gui->slider_float("Offset Z", &globals::visuals::viewmodel::offset[2], -5.f, 5.f);
									}
									gui->end_child();
								}
								gui->end_child();
							}
							gui->end_group();

							gui->sameline();

							gui->begin_group();
							{
								gui->begin_child("Lighting", 0, 1, ImVec2(0, 310));
								{
									gui->label_color_edit("Ambient Color", globals::visuals::ambient_color);
									gui->label_color_edit("Outdoor Ambient", globals::visuals::outdoor_ambient_color);
									gui->label_color_edit("ColorShift Top", globals::visuals::colorshift_top);
									gui->label_color_edit("ColorShift Bottom", globals::visuals::colorshift_bottom);
									gui->slider_float("Brightness", &globals::visuals::brightness, 0.f, 10.f);
									gui->slider_float("Exposure", &globals::visuals::exposure, 0.f, 5.f);
									gui->label_color_edit("Fog Color", globals::visuals::fog_color);
									gui->slider_float("Fog Start", &globals::visuals::fog_start, 0.f, 5000.f);
									gui->slider_float("Fog End", &globals::visuals::fog_end, 0.f, 20000.f);
									gui->slider_float("Geographic Latitude", &globals::visuals::geographic_latitude, -90.f, 90.f);
									gui->warning_checkbox("Force Lighting", &globals::visuals::force_lighting);
									gui->label_color_edit("Shadow Color", globals::visuals::shadow_color);
									gui->checkbox("No Shadows", &globals::visuals::no_shadows, &globals::visuals::no_shadows_key, &globals::visuals::no_shadows_mode);
									gui->slider_float("Diffuse Scale", &globals::visuals::environment_diffuse_scale, 0.f, 5.f);
									gui->slider_float("Specular Scale", &globals::visuals::environment_specular_scale, 0.f, 5.f);

									gui->checkbox("Atmosphere", &globals::visuals::atmosphere::enabled);
									gui->label_color_edit("Atmosphere Color", globals::visuals::atmosphere::color);
									gui->label_color_edit("Atmosphere Decay", globals::visuals::atmosphere::decay);
									gui->slider_float("Atmosphere Density", &globals::visuals::atmosphere::density, 0.f, 1.f);
									gui->slider_float("Atmosphere Glare", &globals::visuals::atmosphere::glare, 0.f, 10.f);
									gui->slider_float("Atmosphere Haze", &globals::visuals::atmosphere::haze, 0.f, 10.f);
									gui->slider_float("Atmosphere Offset", &globals::visuals::atmosphere::offset, 0.f, 1.f);
									
									gui->checkbox("Color Correction", &globals::visuals::color_correction::enabled);
									gui->slider_float("Color Correction Brightness", &globals::visuals::color_correction::brightness, -1.f, 1.f);
									gui->slider_float("Color Correction Contrast", &globals::visuals::color_correction::contrast, -1.f, 1.f);
									gui->slider_float("Color Correction Saturation", &globals::visuals::color_correction::saturation, -1.f, 1.f);
									gui->label_color_edit("Color Correction Tint Color", globals::visuals::color_correction::tint_color);
									
							}
								gui->end_child();

							

								gui->begin_child("World", 0, 1, ImVec2(0, 270));
								{
									gui->checkbox("Bloom Enabled", &globals::visuals::bloom::enabled);
									gui->slider_float("Bloom Intensity", &globals::visuals::bloom::intensity, 0.f, 10.f);
									gui->slider_float("Bloom Size", &globals::visuals::bloom::size, 0.f, 100.f);
									gui->slider_float("Bloom Threshold", &globals::visuals::bloom::threshold, 0.f, 10.f);

									gui->checkbox("Depth Enabled", &globals::visuals::depth_of_field::enabled);
									gui->slider_float("Depth Density", &globals::visuals::depth_of_field::density, 0.f, 1.f);
									gui->slider_float("Depth Focus Distance", &globals::visuals::depth_of_field::focus_distance, 0.f, 1000.f);
									gui->slider_float("Depth In Focus Radius", &globals::visuals::depth_of_field::in_focus_radius, 0.f, 1000.f);
									gui->slider_float("Depth Near Intensity", &globals::visuals::depth_of_field::near_intensity, 0.f, 1.f);
									gui->checkbox("Sunrays Enabled", &globals::visuals::sunrays::enabled);
									gui->slider_float("Sunrays Intensity", &globals::visuals::sunrays::intensity, 0.f, 1.f);
									gui->slider_float("Sunrays Spread", &globals::visuals::sunrays::spread, 0.f, 1.f);
									gui->slider_float("Time of Day", &globals::visuals::time_of_day, 0.f, 24.f);
									gui->slider_int("Star Count", &globals::visuals::star_count, 0, 10000);
									gui->slider_float("Sun Size", &globals::visuals::sun_size, 0.f, 100.f);
									gui->slider_float("Moon Size", &globals::visuals::moon_size, 0.f, 100.f);
									gui->warning_checkbox("Celestial Bodies", &globals::visuals::celestial_bodies);
									
								}
								gui->end_child();
							}
							gui->end_group();
						}

						if (section == 3)
						{
							const char* speed_methods[] = { "Normal", "Velocity", "CFrame", "Velocity (Camera)", "CFrame (Camera)" };
							const char* fly_methods[] = { "Normal", "Velocity (Camera)", "CFrame (Camera)" };
							const char* camera_methods[] = { "Normal", "Smooth" };
							const char* pitch_bases[] = { "None", "Up", "Down", "Zero" };
							const char* yaw_bases[] = { "None", "Backwards", "Left", "Right", "Spin" };
							const char* sound_types[] = { "Default", "Neverlose", "Skeet", "Maim" };
							const char* no_clip_methods[] = { "Normal", "Map Parser" };

							gui->begin_group();
							{
								gui->begin_child("Movement", 2, 1, ImVec2(0, 310));
								{
									gui->warning_checkbox("Speed Hack", &globals::misc::speed_hack, &globals::misc::speed_key, &globals::misc::speed_mode);
									gui->dropdown("Speed Method", &globals::misc::speed_method, speed_methods, IM_ARRAYSIZE(speed_methods));
									gui->slider_float("Speed Value", &globals::misc::speed_value, 0.f, 1000.f);

									gui->warning_checkbox("Fly Hack", &globals::misc::fly_hack, &globals::misc::fly_key, &globals::misc::fly_mode);
									gui->dropdown("Fly Method", &globals::misc::fly_method, fly_methods, IM_ARRAYSIZE(fly_methods));
									gui->slider_float("Fly Speed", &globals::misc::fly_speed, 0.f, 1000.f);

									gui->warning_checkbox("Free Camera", &globals::misc::free_camera, &globals::misc::free_camera_key, &globals::misc::free_camera_mode);
									gui->dropdown("Camera Method", &globals::misc::free_camera_method, camera_methods, IM_ARRAYSIZE(camera_methods));
									gui->slider_float("Camera Speed", &globals::misc::free_camera_speed, 0.f, 1000.f);

									gui->checkbox("Long Neck", &globals::misc::long_neck);
									gui->slider_float("Neck Length", &globals::misc::neck_length, -10.f, 10.f);

									gui->checkbox("No Fall Damage", &globals::misc::no_fall_damage);
									gui->checkbox("Spider Man", &globals::misc::spiderman);
									gui->checkbox("Click TP", &globals::misc::click_tp, &globals::misc::click_tp_key, &globals::misc::click_tp_mode);
									gui->checkbox("Fly Swim", &globals::misc::fly_swim);
									gui->checkbox("Vehicle Fly", &globals::misc::vehicle_fly);
									if (globals::misc::vehicle_fly)
										gui->slider_float("Vehicle Speed", &globals::misc::vehicle_fly_speed, 0.f, 1000.f);

									gui->checkbox("Ideal Peak", &globals::misc::ideal_peak, &globals::misc::ideal_peak_key, &globals::misc::ideal_peak_mode);

									gui->checkbox("No Send", &globals::misc::no_send);
									gui->checkbox("Anti AFK", &globals::misc::anti_afk);
								}
								gui->end_child();

								gui->begin_child("Angles", 2, 1, ImVec2(0, 310));
								{
									gui->warning_checkbox("Angles Enabled", &globals::misc::angles_enabled, &globals::misc::angles_key, &globals::misc::angles_mode);

									gui->dropdown("Pitch Base", &globals::misc::pitch_base, pitch_bases, IM_ARRAYSIZE(pitch_bases));
									gui->slider_float("Pitch Value", &globals::misc::pitch_value, -90.f, 90.f);

									gui->dropdown("Yaw Base", &globals::misc::yaw_base, yaw_bases, IM_ARRAYSIZE(yaw_bases));
									gui->slider_float("Yaw Value", &globals::misc::yaw_value, -180.f, 180.f);

									gui->checkbox("Yaw Jitter", &globals::misc::yaw_jitter);

									if (globals::misc::yaw_jitter)
										gui->slider_float("Jitter Value", &globals::misc::jitter_value, 0.f, 180.f);

									gui->checkbox("Roll", &globals::misc::roll);
								}
								gui->end_child();
							}
							gui->end_group();

							gui->sameline();

							gui->begin_group();
							{
								gui->begin_child("Manipulation", 0, 1, ImVec2(0, 310));
								{
									gui->warning_checkbox("Jump Power", &globals::misc::jump_power_enabled, &globals::misc::jump_power_key, &globals::misc::jump_power_mode);
									gui->slider_float("Jump Power Value", &globals::misc::jump_power_value, 0.f, 500.f);

									gui->warning_checkbox("Gravity", &globals::misc::gravity_enabled, &globals::misc::gravity_key, &globals::misc::gravity_mode);
									gui->slider_float("Gravity Value", &globals::misc::gravity_value, 0.f, 1000.f);

									gui->warning_checkbox("Hip Height", &globals::misc::hip_height_enabled, &globals::misc::hip_height_key, &globals::misc::hip_height_mode);
									gui->slider_float("Hip Height Value", &globals::misc::hip_height_value, -5.f, 20.f);
									gui->checkbox("Desync", &globals::misc::desync, &globals::misc::desync_key, &globals::misc::desync_mode);

									gui->warning_checkbox("Tickrate Manipulation", &globals::misc::tickrate_manipulation);
									if (globals::misc::tickrate_manipulation)
										gui->slider_float("Tickrate Value", &globals::misc::tickrate_value, 0.0f, 1000.f, false, "%.0f FPS");

									gui->checkbox("Sit", &globals::misc::sit);

									gui->warning_checkbox("Platform Stand", &globals::misc::platform_stand);

									gui->warning_checkbox("Jump Bug", &globals::misc::jump_bug, &globals::misc::jump_bug_key, &globals::misc::jump_bug_mode);

									gui->checkbox("No Rotate", &globals::misc::no_rotate);

									gui->warning_checkbox("No Clip", &globals::misc::no_clip, &globals::misc::no_clip_key, &globals::misc::no_clip_mode);
									gui->dropdown("No Clip Method", &globals::misc::no_clip_method, no_clip_methods, IM_ARRAYSIZE(no_clip_methods));
									gui->warning_checkbox("Camera Offset", &globals::misc::camera_offset_enabled);
									gui->slider_float("Camera Offset X", &globals::misc::camera_offset[0], -10.f, 10.f);
									gui->slider_float("Camera Offset Y", &globals::misc::camera_offset[1], -10.f, 10.f);
									gui->slider_float("Camera Offset Z", &globals::misc::camera_offset[2], -10.f, 10.f);
									gui->slider_float("Min Zoom", &globals::misc::camera_zoom[0], 0.f, 2000.f);
									gui->slider_float("Max Zoom", &globals::misc::camera_zoom[1], 0.f, 2000.f);
									gui->slider_float("FOV Override", &globals::misc::fov, 30.f, 120.f);
									gui->warning_checkbox("Zoom Enabled", &globals::misc::zoom_enabled, &globals::misc::zoom_key, &globals::misc::zoom_mode);
								}
								gui->end_child();

								gui->begin_child("Miscellaneous", 0, 1, ImVec2(0, 270));
								{
									gui->checkbox("Anchor on Bind", &globals::misc::anchor_on_bind, &globals::misc::anchor_key, &globals::misc::anchor_mode);

									gui->checkbox("Waypoint on Bind", &globals::misc::waypoint_on_bind, &globals::misc::waypoint_key, &globals::misc::waypoint_mode);

									gui->checkbox("Waypoint on Respawn", &globals::misc::waypoint_on_respawn);

									gui->warning_checkbox("Hit Sounds", &globals::misc::hit_sounds, &globals::misc::hit_sounds_key, &globals::misc::hit_sounds_mode);
									gui->dropdown("Hit Sound Type", &globals::misc::hit_sound_type, sound_types, IM_ARRAYSIZE(sound_types));

									gui->warning_checkbox("Death Sounds", &globals::misc::death_sounds, &globals::misc::death_sounds_key, &globals::misc::death_sounds_mode);
									gui->dropdown("Death Sound Type", &globals::misc::death_sound_type, sound_types, IM_ARRAYSIZE(sound_types));

									gui->warning_checkbox("Ping Spike", &globals::misc::ping_spike, &globals::misc::ping_spike_key, &globals::misc::ping_spike_mode);
									gui->slider_float("Spike Value", &globals::misc::ping_spike_value, 0.f, 1000.f, "%.0fms");

									ImGui::Spacing();
									gui->checkbox("Keybind List", &var->keybind_list_enabled);
								}
								gui->end_child();


							}
							gui->end_group();

					}
					gui->end_content();
				}
				gui->end();
			}


			if (var->gui.current_section[1])
			{
				gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
				gui->begin("other", nullptr, var->window.flags);
				{
					draw->window_decorations();

					gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 1));
					gui->begin_content();
					{
						gui->begin_group();
						{
							static int selected_entry = -1;
							static std::uint64_t selected_player_address = 0;

							std::vector<rbx::player_t> players;
							if (globals::players.address)
								players = globals::players.get_children<rbx::player_t>();

							ImVec2 entries_size = ImVec2(GetContentRegionAvail().x - 1, GetContentRegionAvail().y - 86);
							gui->begin_def_child("Entries Panel", entries_size, ImGuiChildFlags_None, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysUseWindowPadding);
							{
								ImVec2 header_min = GetWindowPos() + ImVec2(2, 2);
								ImVec2 header_max = header_min + ImVec2(GetWindowSize().x - 4, elements->section.height);
								draw->fade_rect_filled(GetWindowDrawList(), header_min, header_max, draw->get_clr(clr->window.background_two), draw->get_clr(clr->window.background_one), fade_direction::vertically);
								draw->line(GetWindowDrawList(), GetWindowPos() + ImVec2(2, 2), GetWindowPos() + ImVec2(GetWindowSize().x - 2, 2), draw->get_clr(clr->accent));
								draw->line(GetWindowDrawList(), GetWindowPos() + ImVec2(2, 3), GetWindowPos() + ImVec2(GetWindowSize().x - 2, 3), draw->get_clr(clr->accent, 0.4f));
								draw->rect(GetWindowDrawList(), GetWindowPos() + ImVec2(1, 1), GetWindowPos() + GetWindowSize() - ImVec2(1, 1), draw->get_clr(clr->window.stroke));
								draw->text_outline(GetWindowDrawList(), var->font.tahoma, var->font.tahoma->FontSize, header_min + ImVec2(6, 2), draw->get_clr(clr->widgets.text), "Options");

								gui->set_cursor_pos(ImVec2(2, elements->section.height + 4));

								gui->push_font(var->font.tahoma);
								gui->push_style_color(ImGuiCol_TableBorderLight, draw->get_clr(clr->window.stroke));
								gui->push_style_color(ImGuiCol_TableBorderStrong, draw->get_clr(clr->window.stroke));
								gui->push_style_color(ImGuiCol_TableRowBg, draw->get_clr(clr->window.background_one));
								gui->push_style_color(ImGuiCol_TableRowBgAlt, draw->get_clr(clr->window.background_one));
								gui->push_style_color(ImGuiCol_Text, draw->get_clr(clr->widgets.text_inactive));
								if (gui->begin_table("EntriesTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(GetContentRegionAvail().x - 3, GetContentRegionAvail().y - 6)))
								{
									for (int row = 0; row < (int)players.size(); row++)
									{
										auto& player = players[row];
										if (player.address == 0) continue;

										std::string name = player.get_name();
										std::string display_name = player.get_display_name();
										std::string final_name = globals::visuals::options::use_display_name ? display_name : name;

										gui->table_next_row();
										gui->table_set_column_index(0);

										bool is_selected = selected_player_address == player.address;
										if (is_selected) gui->push_style_color(ImGuiCol_Text, draw->get_clr(clr->accent));
										draw->text_outline(final_name.c_str());
										if (is_selected) gui->pop_style_color();

										if (IsItemClicked()) {
											selected_entry = row;
											selected_player_address = player.address;
										}

										gui->table_set_column_index(1);
										draw->text_outline("Player");

										gui->table_set_column_index(2);
										{
											int status = 0;
											{
												std::lock_guard<std::mutex> lock(globals::player_overrides_mtx);
												if (globals::player_overrides.count(player.address)) {
													status = globals::player_overrides[player.address];
												} else {
													if (player.address == globals::local_player.address) {
														status = 2;
													} else {
														if (globals::custom_game::is_detected) {
															auto get_custom_team = [&](rbx::player_t p) {
																auto model = p.get_model_instance();
																if (!model.address) return 0;
																
																std::uint64_t parent = memory->read<std::uint64_t>(model.address + Offsets::Instance::Parent);
																if (parent == globals::custom_game::terrorist_folder.address) return 1;
																if (parent == globals::custom_game::counter_terrorist_folder.address) return 2;
																
																if (parent != 0) {
																	std::uint64_t grandparent = memory->read<std::uint64_t>(parent + Offsets::Instance::Parent);
																	if (grandparent == globals::custom_game::terrorist_folder.address) return 1;
																	if (grandparent == globals::custom_game::counter_terrorist_folder.address) return 2;
																}
																return 0;
															};

															int p_custom_team = get_custom_team(player);

															if (globals::custom_game::local_team != 0 && globals::custom_game::local_team == p_custom_team) {
																status = 1;
															}
															else {
																status = 0;
															}
														}
														else {
															rbx::instance_t local_team = rbx::player_t(globals::local_player.address).get_team();
															rbx::instance_t player_team = player.get_team();
															if (local_team.address != 0 && local_team.address == player_team.address) {
																status = 1;
															}
															else {
																status = 0;
															}
														}
													}
												}
											}

											const char* status_text = "Enemy";
											ImU32 status_clr = draw->get_clr(clr->widgets.text_inactive);

											if (status == 1) {
												status_text = "Friendly";
												status_clr = IM_COL32(0, 255, 0, 255);
											} else if (status == 2) {
												status_text = "Client";
												status_clr = draw->get_clr(clr->accent);
											}

											gui->push_style_color(ImGuiCol_Text, status_clr);
											draw->text_outline(status_text);
											gui->pop_style_color();

											if (IsItemClicked()) {
												std::lock_guard<std::mutex> lock(globals::player_overrides_mtx);
												int next_status = (status + 1) % 3;
												globals::player_overrides[player.address] = next_status;
											}
										}
									}
									gui->end_table();
								}
								gui->pop_style_color(5);
								gui->pop_font();
							}
							gui->end_def_child();

							ImVec2 panel_min = GetCursorScreenPos() + ImVec2(0, 6);
							ImVec2 panel_max = panel_min + ImVec2(GetContentRegionAvail().x - 1, 70);
							draw->rect(GetWindowDrawList(), panel_min, panel_max, draw->get_clr(clr->widgets.stroke_two));
							draw->rect(GetWindowDrawList(), panel_min + ImVec2(1, 1), panel_max - ImVec2(1, 1), draw->get_clr(clr->window.stroke));
							draw->rect_filled(GetWindowDrawList(), panel_min + ImVec2(2, 2), panel_max - ImVec2(2, 2), draw->get_clr(clr->window.background_one));
							draw->line(GetWindowDrawList(), panel_min + ImVec2(2, 2), ImVec2(panel_max.x - 2, panel_min.y + 2), draw->get_clr(clr->accent));
							draw->line(GetWindowDrawList(), panel_min + ImVec2(2, 3), ImVec2(panel_max.x - 2, panel_min.y + 3), draw->get_clr(clr->accent, 0.4f));

							float panel_h = panel_max.y - panel_min.y;
							float img_size = panel_h - 16.0f;
							ImVec2 img_min = panel_min + ImVec2(8, 8);
							ImVec2 img_max = img_min + ImVec2(img_size, img_size);
							draw->rect(GetWindowDrawList(), img_min, img_max, draw->get_clr(clr->widgets.stroke_two));
							draw->rect(GetWindowDrawList(), img_min + ImVec2(1, 1), img_max - ImVec2(1, 1), draw->get_clr(clr->window.stroke));
							draw->rect_filled(GetWindowDrawList(), img_min + ImVec2(2, 2), img_max - ImVec2(2, 2), draw->get_clr(clr->window.background_two));

							ImVec2 inner_local_pos = ImVec2(img_min.x + 4 - GetWindowPos().x, img_min.y + 4 - GetWindowPos().y);
							ImVec2 inner_size = ImVec2((img_max.x - img_min.x) - 8, (img_max.y - img_min.y) - 8);
							gui->set_cursor_pos(inner_local_pos);
							gui->begin_def_child("preview box", inner_size, ImGuiChildFlags_None, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysUseWindowPadding);
							{
								gui->push_font(var->font.tahoma);

								std::uint64_t userId = 0;
								if (selected_player_address != 0) {
									userId = memory->read<std::uint64_t>(selected_player_address + Offsets::Player::UserId);
								}

								ID3D11ShaderResourceView* headshot = userId != 0 ? load_texture_from_url(userId) : nullptr;

								if (headshot) {
									ImGui::Image((void*)headshot, inner_size - ImVec2(8, 8));
								} else {
									gui->set_cursor_pos(ImVec2(6, 6));
									draw->text_outline("..?");
								}

								gui->pop_font();
							}
							gui->end_def_child();


						}
						gui->end_group();
					}
					gui->end_content();
				}
				gui->end();
			}


			if (var->gui.current_section[2])
			{
				gui->set_next_window_size_constraints(ImVec2(314, 499), GetIO().DisplaySize);
				gui->begin("Preview", nullptr, var->window.flags);
				{
					draw->window_decorations();

					gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 1));
					gui->begin_group();
					{
						gui->begin_child("PreviewBody", 2, 2, ImVec2(284, 350));
						{
						}
						gui->end_child();

						ImGui::Spacing();
						ImGui::Spacing();

						gui->begin_child("Conditions", 2, 2, ImVec2(284, 80));
						{
							static int cond = 0;
							const char* items[] = { "None", "Visible", "Distance" };
							gui->dropdown("Conditions", &cond, items, IM_ARRAYSIZE(items));
						}
						gui->end_child();
					}
					gui->end_group();
				}
				gui->end();

			}

			if (var->gui.current_section[3])
			{
				gui->set_next_window_size_constraints(ImVec2(400, 480), GetIO().DisplaySize);
				gui->begin("Waypoints", nullptr, var->window.flags);
				{
					draw->window_decorations();
					gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 1));
					gui->begin_content();
					{
						gui->begin_child("Waypoints List", 1, 1, ImVec2(0, 220));
						{
							for (int i = 0; i < (int)globals::misc::waypoints.size(); i++)
							{
								bool selected = (globals::misc::selected_waypoint == i);
								if (gui->selectable(globals::misc::waypoints[i].name.c_str(), &selected, ImVec2(0, 18)))
								{
									globals::misc::selected_waypoint = i;
									strcpy_s(globals::misc::waypoint_name_buffer, globals::misc::waypoints[i].name.c_str());
								}
							}
						}
						gui->end_child();

						gui->begin_child("Waypoint Controls", 1, 1, ImVec2(0, 220));
						{
							gui->text_field("Waypoint Name", globals::misc::waypoint_name_buffer, 64);

							if (gui->button("Create", 1)) {
								if (strlen(globals::misc::waypoint_name_buffer) > 0) {
									cache::entity_t local;
									{ std::lock_guard<std::mutex> lock(cache::mtx); local = cache::cached_local_player; }
									if (local.root_part.part.address) {
										rbx::vector3_t pos = memory->read<rbx::vector3_t>(local.root_part.part.address + Offsets::BasePart::Position);
										globals::misc::waypoints.push_back({ globals::misc::waypoint_name_buffer, pos, 0 });
									}
								}
							}
							gui->sameline();
							if (gui->button("Delete", 1)) {
								if (globals::misc::selected_waypoint != -1 && globals::misc::selected_waypoint < (int)globals::misc::waypoints.size()) {
									globals::misc::waypoints.erase(globals::misc::waypoints.begin() + globals::misc::selected_waypoint);
									globals::misc::selected_waypoint = -1;
									globals::misc::waypoint_name_buffer[0] = '\0';
								}
							}

							if (gui->button("Teleport", 1)) {
								if (globals::misc::selected_waypoint != -1 && globals::misc::selected_waypoint < (int)globals::misc::waypoints.size()) {
									cache::entity_t local;
									{ std::lock_guard<std::mutex> lock(cache::mtx); local = cache::cached_local_player; }
									if (local.root_part.part.address) {
										memory->write<rbx::vector3_t>(local.root_part.part.address + Offsets::BasePart::Position, globals::misc::waypoints[globals::misc::selected_waypoint].pos);
									}
								}
							}
							gui->sameline();
							if (gui->button("Save Current Pos", 1)) {
								if (globals::misc::selected_waypoint != -1 && globals::misc::selected_waypoint < (int)globals::misc::waypoints.size()) {
									cache::entity_t local;
									{ std::lock_guard<std::mutex> lock(cache::mtx); local = cache::cached_local_player; }
									if (local.root_part.part.address) {
										globals::misc::waypoints[globals::misc::selected_waypoint].pos = memory->read<rbx::vector3_t>(local.root_part.part.address + Offsets::BasePart::Position);
									}
								}
							}

							if (globals::misc::selected_waypoint != -1 && globals::misc::selected_waypoint < (int)globals::misc::waypoints.size()) {
								auto& wp = globals::misc::waypoints[globals::misc::selected_waypoint];
								static int dummy_mode = 0;
								gui->keybind("Teleport Bind", &wp.keybind, &dummy_mode);
							}
						}
						gui->end_child();
					}
					gui->end_content();
				}
				gui->end();
			}

			if (var->gui.current_section[4])
			{
				gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
				gui->begin("Explore", nullptr, var->window.flags);
				{
					draw->window_decorations();

					gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 1));
					gui->begin_content();
					{
						gui->push_font(var->font.tahoma);

						static char explorer_search[64] = "";
						static rbx::instance_t selected_instance{};

						gui->begin_group();
						{
							float explorer_width = (GetContentRegionAvail().x - 1) * 0.6f;
							ImVec2 explorer_size = ImVec2(explorer_width, GetContentRegionAvail().y);

							gui->begin_def_child("Explorer", explorer_size, ImGuiChildFlags_ResizeX, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
							{
								ImVec2 header_min = GetWindowPos() + ImVec2(2, 2);
								ImVec2 header_max = header_min + ImVec2(GetWindowSize().x - 4, elements->section.height);
								draw->fade_rect_filled(GetWindowDrawList(), header_min, header_max, draw->get_clr(clr->window.background_two), draw->get_clr(clr->window.background_one), fade_direction::vertically);
								draw->line(GetWindowDrawList(), GetWindowPos() + ImVec2(2, 2), GetWindowPos() + ImVec2(GetWindowSize().x - 2, 2), draw->get_clr(clr->accent));
								draw->line(GetWindowDrawList(), GetWindowPos() + ImVec2(2, 3), GetWindowPos() + ImVec2(GetWindowSize().x - 2, 3), draw->get_clr(clr->accent, 0.4f));
								draw->rect(GetWindowDrawList(), GetWindowPos() + ImVec2(1, 1), GetWindowPos() + GetWindowSize() - ImVec2(1, 1), draw->get_clr(clr->window.stroke));
								draw->text_outline(GetWindowDrawList(), var->font.tahoma, var->font.tahoma->FontSize, header_min + ImVec2(6, 2), draw->get_clr(clr->widgets.text), "Explorer");

								gui->set_cursor_pos(ImVec2(8, elements->section.height + 8));

								gui->push_style_var(ImGuiStyleVar_FrameRounding, 2.f);
								gui->text_field("Search...", explorer_search, IM_ARRAYSIZE(explorer_search));
								gui->pop_style_var();

								gui->set_cursor_pos(ImVec2(4, GetCursorPos().y + 4));

								if (globals::datamodel.address)
								{
									std::function<void(rbx::instance_t)> render_tree;
									render_tree = [&](rbx::instance_t instance) {
										std::string name = instance.get_name();
										std::string class_name = instance.get_class_name();

										if (explorer_search[0] != '\0') {
											std::string lower_name = name;
											std::string lower_class = class_name;
											std::string lower_search = explorer_search;
											std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
											std::transform(lower_class.begin(), lower_class.end(), lower_class.begin(), ::tolower);
											std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(), ::tolower);

											if (lower_name.find(lower_search) == std::string::npos &&
												lower_class.find(lower_search) == std::string::npos) {
											}
										}

										std::string label = name + " (" + class_name + ")";

										ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
										if (selected_instance.address == instance.address)
											flags |= ImGuiTreeNodeFlags_Selected;

										bool has_children = instance.has_children();
										if (!has_children)
											flags |= ImGuiTreeNodeFlags_Leaf;

										bool opened = ImGui::TreeNodeEx((void*)instance.address, flags, "%s", label.c_str());

										if (ImGui::IsItemClicked())
											selected_instance = instance;

										if (opened) {
											if (has_children) {
												for (auto& child : instance.get_children()) {
													render_tree(child);
												}
											}
											ImGui::TreePop();
										}
										};

									render_tree(globals::datamodel);
								}
							}
							gui->end_def_child();

							gui->sameline();

							ImVec2 properties_size = ImVec2(GetContentRegionAvail().x - 1, GetContentRegionAvail().y);
							gui->begin_def_child("Properties", properties_size, ImGuiChildFlags_None, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding);
							{
								ImVec2 header_min = GetWindowPos() + ImVec2(2, 2);
								ImVec2 header_max = header_min + ImVec2(GetWindowSize().x - 4, elements->section.height);
								draw->fade_rect_filled(GetWindowDrawList(), header_min, header_max, draw->get_clr(clr->window.background_two), draw->get_clr(clr->window.background_one), fade_direction::vertically);
								draw->line(GetWindowDrawList(), GetWindowPos() + ImVec2(2, 2), GetWindowPos() + ImVec2(GetWindowSize().x - 2, 2), draw->get_clr(clr->accent));
								draw->line(GetWindowDrawList(), GetWindowPos() + ImVec2(2, 3), GetWindowPos() + ImVec2(GetWindowSize().x - 2, 3), draw->get_clr(clr->accent, 0.4f));
								draw->rect(GetWindowDrawList(), GetWindowPos() + ImVec2(1, 1), GetWindowPos() + GetWindowSize() - ImVec2(1, 1), draw->get_clr(clr->window.stroke));
								draw->text_outline(GetWindowDrawList(), var->font.tahoma, var->font.tahoma->FontSize, header_min + ImVec2(6, 2), draw->get_clr(clr->widgets.text), "Properties");

								gui->set_cursor_pos(ImVec2(8, elements->section.height + 8));

								if (selected_instance.address != 0)
								{
									std::string name = selected_instance.get_name();
									std::string class_name = selected_instance.get_class_name();
									char addr_buf[32]; snprintf(addr_buf, sizeof(addr_buf), "0x%llX", selected_instance.address);

									draw->text_outline("Name: "); gui->sameline();
									gui->push_style_color(ImGuiCol_Text, draw->get_clr(clr->accent));
									draw->text_outline(name.c_str()); gui->pop_style_color();

									draw->text_outline("Class: "); gui->sameline();
									gui->push_style_color(ImGuiCol_Text, draw->get_clr(clr->accent));
									draw->text_outline(class_name.c_str()); gui->pop_style_color();

									draw->text_outline("Address: "); gui->sameline();
									gui->push_style_color(ImGuiCol_Text, draw->get_clr(clr->accent));
									draw->text_outline(addr_buf); gui->pop_style_color();

									ImGui::Spacing();
									ImGui::Separator();
									ImGui::Spacing();

									if (gui->button("Copy Name")) {
										ImGui::SetClipboardText(name.c_str());
									}
									if (gui->button("Copy Class")) {
										ImGui::SetClipboardText(class_name.c_str());
									}
									if (gui->button("Copy Address")) {
										ImGui::SetClipboardText(addr_buf);
									}
								}
								else
								{
									draw->text_outline("Select an object to view properties.");
								}
							}
							gui->end_def_child();
						}
						gui->end_group();

						gui->pop_font();
					}
					gui->end_content();
				}
				gui->end();
			}

			if (var->gui.current_section[5])
			{
				gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
				gui->begin("Style", nullptr, var->window.flags);
				{
					draw->window_decorations();

					gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 1));
					gui->begin_content();
					{
						static float menu_accent[4];
						static float contrast_one[4];
						static float contrast_two[4];
						static float inline_c[4];
						static float outline_c[4];
						static float text_active[4];
						static float text_inactive[4];
						static float text_outline_c[4];
						static float warning_c[4];

						static bool initialized = false;
						if (!initialized)
						{
							menu_accent[0] = clr->accent.Value.x;
							menu_accent[1] = clr->accent.Value.y;
							menu_accent[2] = clr->accent.Value.z;
							menu_accent[3] = 1.f;

							contrast_one[0] = clr->window.background_one.Value.x;
							contrast_one[1] = clr->window.background_one.Value.y;
							contrast_one[2] = clr->window.background_one.Value.z;
							contrast_one[3] = 1.f;

							contrast_two[0] = clr->window.background_two.Value.x;
							contrast_two[1] = clr->window.background_two.Value.y;
							contrast_two[2] = clr->window.background_two.Value.z;
							contrast_two[3] = 1.f;

							inline_c[0] = clr->window.stroke.Value.x;
							inline_c[1] = clr->window.stroke.Value.y;
							inline_c[2] = clr->window.stroke.Value.z;
							inline_c[3] = 1.f;

							outline_c[0] = clr->widgets.stroke_two.Value.x;
							outline_c[1] = clr->widgets.stroke_two.Value.y;
							outline_c[2] = clr->widgets.stroke_two.Value.z;
							outline_c[3] = 1.f;

							text_active[0] = clr->widgets.text.Value.x;
							text_active[1] = clr->widgets.text.Value.y;
							text_active[2] = clr->widgets.text.Value.z;
							text_active[3] = 1.f;

							text_inactive[0] = clr->widgets.text_inactive.Value.x;
							text_inactive[1] = clr->widgets.text_inactive.Value.y;
							text_inactive[2] = clr->widgets.text_inactive.Value.z;
							text_inactive[3] = 1.f;

							text_outline_c[0] = clr->widgets.text_outline.Value.x;
							text_outline_c[1] = clr->widgets.text_outline.Value.y;
							text_outline_c[2] = clr->widgets.text_outline.Value.z;
							text_outline_c[3] = 1.f;

							warning_c[0] = clr->widgets.warning.Value.x;
							warning_c[1] = clr->widgets.warning.Value.y;
							warning_c[2] = clr->widgets.warning.Value.z;
							warning_c[3] = 1.f;

							initialized = true;
						}

						{
							ImGuiID theme_id = GetCurrentWindow()->GetID("Style Subtabs");
							style_subtab = gui->get_child_subtab(theme_id);

							gui->begin_multi_subtab(
								"Style Subtabs",
								1,
								1,
								2,
								ImVec2(0, 180),
								{ "Themes", "Extra" }
							);

							if (style_subtab == 0)
							{
								static int previous_theme = -1;
								const char* themes[] = { "Default", "Atlanta", "Blue", "Pink", "Purple", "White", "Red", "Green", "Yellow", "Gray", "Teal Dark", "Redv2",
									"Abyss","Fatality","Neverlose","Aimware","Youtube","Gamesense","Onetap","Entropy","Interwebz","Dracula","Spotify","Sublime","Vape","Neko","Corn","Minecraft", "auqa", "White Purple" };


								if (var->gui.theme_index != previous_theme)
								{
									auto hex = [](const char* s) -> ImColor {
										unsigned int rgb = 0;
										sscanf_s(s, "%x", &rgb);
										int r = (rgb >> 16) & 0xFF;
										int g = (rgb >> 8) & 0xFF;
										int b = rgb & 0xFF;
										return ImColor(r, g, b);
										};
									if (var->gui.theme_index == 1)
									{
										clr->accent = ImColor(154, 127, 172);
										clr->window.background_one = ImColor(36, 36, 47);
									}
									if (var->gui.theme_index == 2)
									{
										clr->accent = ImColor(62, 93, 241);
										clr->window.background_one = ImColor(12, 12, 12);
									}
									if (var->gui.theme_index == 3)
									{
										clr->accent = ImColor(204, 115, 146);
										clr->window.background_one = ImColor(19, 19, 21);

									}
									if (var->gui.theme_index == 4)
									{
										clr->accent = ImColor(141, 0, 234);
										clr->window.background_one = ImColor(0, 0, 0);
									}
									if (var->gui.theme_index == 5)
									{
										clr->accent = ImColor(255, 255, 255);
										clr->window.background_one = ImColor(12, 12, 12);
									}
									if (var->gui.theme_index == 6)
									{
										clr->accent = ImColor(255, 50, 50);
										clr->window.background_one = ImColor(20, 0, 0);
									}
									if (var->gui.theme_index == 7)
									{
										clr->accent = ImColor(50, 255, 120);
										clr->window.background_one = ImColor(0, 20, 10);
									}
									if (var->gui.theme_index == 8)
									{
										clr->accent = ImColor(255, 255, 100);
										clr->window.background_one = ImColor(30, 30, 0);
									}
									if (var->gui.theme_index == 9)
									{
										clr->accent = ImColor(160, 160, 160);
										clr->window.background_one = ImColor(25, 25, 25);
									}
									if (var->gui.theme_index == 10)
									{
										clr->accent = ImColor(95, 144, 154);
										clr->window.background_one = ImColor(20, 20, 20);
										clr->window.background_two = ImColor(30, 30, 30);
										clr->window.stroke = ImColor(45, 45, 45);
										clr->widgets.stroke_two = ImColor(10, 10, 10);
										clr->widgets.text = ImColor(180, 180, 180);
										clr->widgets.text_inactive = ImColor(136, 136, 136);
									}
									if (var->gui.theme_index == 11)
									{
										clr->accent = ImColor(141, 41, 81);
										clr->window.background_one = ImColor(26, 10, 19);
										clr->window.background_two = ImColor(39, 19, 27);
										clr->window.stroke = ImColor(39, 19, 27);
										clr->widgets.stroke_two = ImColor(39, 19, 27);
										clr->widgets.text = ImColor(200, 200, 200);
										clr->widgets.text_inactive = ImColor(136, 136, 136);
									}
									if (var->gui.theme_index == 12) {
										clr->widgets.stroke_two = hex("0a0a0a");
										clr->accent = hex("8c87b4");
										clr->widgets.text = hex("ffffff");
										clr->widgets.text_inactive = hex("afafaf");
										clr->window.background_one = hex("1e1e1e");
										clr->window.background_two = hex("141414");
										clr->window.stroke = hex("2d2d2d");
									}
									if (var->gui.theme_index == 13) {
										clr->widgets.stroke_two = hex("0f0f28");
										clr->accent = hex("f00f50");
										clr->widgets.text = hex("c8c8ff");
										clr->widgets.text_inactive = hex("afafaf");
										clr->window.background_one = hex("231946");
										clr->window.background_two = hex("191432");
										clr->window.stroke = hex("322850");
									}
									if (var->gui.theme_index == 14) {
										clr->widgets.stroke_two = hex("000005");
										clr->accent = hex("00b4f0");
										clr->widgets.text = hex("ffffff");
										clr->widgets.text_inactive = hex("afafaf");
										clr->window.background_one = hex("000f1e");
										clr->window.background_two = hex("050514");
										clr->window.stroke = hex("0a1e28");
									}
									if (var->gui.theme_index == 15) {
										clr->widgets.stroke_two = hex("000005");
										clr->accent = hex("c82828");
										clr->widgets.text = hex("e8e8e8");
										clr->widgets.text_inactive = hex("afafaf");
										clr->window.background_one = hex("2b2b2b");
										clr->window.background_two = hex("191919");
										clr->window.stroke = hex("373737");
									}
									if (var->gui.theme_index == 16) {
										clr->widgets.stroke_two = hex("000000");
										clr->accent = hex("ff0000");
										clr->widgets.text = hex("f1f1f1");
										clr->widgets.text_inactive = hex("aaaaaa");
										clr->window.background_one = hex("232323");
										clr->window.background_two = hex("0f0f0f");
										clr->window.stroke = hex("393939");
									}
									if (var->gui.theme_index == 17) {
										clr->widgets.stroke_two = hex("000000");
										clr->accent = hex("a7d94d");
										clr->widgets.text = hex("ffffff");
										clr->widgets.text_inactive = hex("afafaf");
										clr->window.background_one = hex("171717");
										clr->window.background_two = hex("0c0c0c");
										clr->window.stroke = hex("282828");
									}
									if (var->gui.theme_index == 18) {
										clr->widgets.stroke_two = hex("000000");
										clr->accent = hex("dda85d");
										clr->widgets.text = hex("d6d9e0");
										clr->widgets.text_inactive = hex("afafaf");
										clr->window.background_one = hex("2c3037");
										clr->window.background_two = hex("1f2125");
										clr->window.stroke = hex("4e5158");
									}
									if (var->gui.theme_index == 19) {
										clr->widgets.stroke_two = hex("0a0a0a");
										clr->accent = hex("81bbe9");
										clr->widgets.text = hex("dcdcdc");
										clr->widgets.text_inactive = hex("afafaf");
										clr->window.background_one = hex("3d3a43");
										clr->window.background_two = hex("302f37");
										clr->window.stroke = hex("4c4a52");
									}
									if (var->gui.theme_index == 20) {
										clr->widgets.stroke_two = hex("1a1a1a");
										clr->accent = hex("c9654b");
										clr->widgets.text = hex("fcfcfc");
										clr->widgets.text_inactive = hex("a8a8a8");
										clr->window.background_one = hex("291f38");
										clr->window.background_two = hex("1f162b");
										clr->window.stroke = hex("40364f");
									}
									if (var->gui.theme_index == 21) {
										clr->widgets.stroke_two = hex("202126");
										clr->accent = hex("9a81b3");
										clr->widgets.text = hex("b4b4b8");
										clr->widgets.text_inactive = hex("88888b");
										clr->window.background_one = hex("2a2c38");
										clr->window.background_two = hex("252730");
										clr->window.stroke = hex("3c384d");
									}
									if (var->gui.theme_index == 22) {
										clr->widgets.stroke_two = hex("0a0a0a");
										clr->accent = hex("1ed760");
										clr->widgets.text = hex("d0d0d0");
										clr->widgets.text_inactive = hex("949494");
										clr->window.background_one = hex("181818");
										clr->window.background_two = hex("121212");
										clr->window.stroke = hex("292929");
									}
									if (var->gui.theme_index == 23) {
										clr->widgets.stroke_two = hex("000000");
										clr->accent = hex("ff9800");
										clr->widgets.text = hex("e8ffff");
										clr->widgets.text_inactive = hex("d3d3c2");
										clr->window.background_one = hex("32332d");
										clr->window.background_two = hex("282923");
										clr->window.stroke = hex("484944");
									}
									if (var->gui.theme_index == 24) {
										clr->widgets.stroke_two = hex("0a0a0a");
										clr->accent = hex("26866a");
										clr->widgets.text = hex("dcdcdc");
										clr->widgets.text_inactive = hex("afafaf");
										clr->window.background_one = hex("1f1f1f");
										clr->window.background_two = hex("1a1a1a");
										clr->window.stroke = hex("363636");
									}
									if (var->gui.theme_index == 25) {
										clr->widgets.stroke_two = hex("000000");
										clr->accent = hex("d21f6a");
										clr->widgets.text = hex("ffffff");
										clr->widgets.text_inactive = hex("afafaf");
										clr->window.background_one = hex("171717");
										clr->window.background_two = hex("131313");
										clr->window.stroke = hex("2d2d2d");
									}
									if (var->gui.theme_index == 26) {
										clr->widgets.stroke_two = hex("000000");
										clr->accent = hex("ff9000");
										clr->widgets.text = hex("dcdcdc");
										clr->widgets.text_inactive = hex("afafaf");
										clr->window.background_one = hex("252525");
										clr->window.background_two = hex("191919");
										clr->window.stroke = hex("333333");
									}
									if (var->gui.theme_index == 27) {
										clr->widgets.stroke_two = hex("000000");
										clr->accent = hex("27ce40");
										clr->widgets.text = hex("ffffff");
										clr->widgets.text_inactive = hex("d7d7d7");
										clr->window.background_one = hex("333333");
										clr->window.background_two = hex("262626");
										clr->window.stroke = hex("333333");
									}
									if (var->gui.theme_index == 28) {
										clr->accent = ImColor(0.71140939f, 1.0f, 0.80825186f);
										clr->window.background_one = ImColor(0.13422817f, 0.13422817f, 0.13422817f);
										clr->window.background_two = ImColor(0.24161077f, 0.24161077f, 0.24161077f);
										clr->window.stroke = ImColor(0.40268457f, 0.40268457f, 0.40268457f);
										clr->widgets.stroke_two = ImColor(0.20000000f, 0.20000000f, 0.20000000f);
										clr->widgets.text = ImColor(1.0f, 1.0f, 1.0f);
										clr->widgets.text_inactive = ImColor(0.69999999f, 0.69999999f, 0.69999999f);
									}
									if (var->gui.theme_index == 29) {
										clr->accent = hex("a855f7");
										clr->window.background_one = hex("ffffff");
										clr->window.background_two = hex("f9fafb");
										clr->window.stroke = hex("e5e7eb");
										clr->widgets.stroke_two = hex("d1d5db");
										clr->widgets.text = hex("111827");
										clr->widgets.text_inactive = hex("6b7280");
									}


									menu_accent[0] = clr->accent.Value.x;
									menu_accent[1] = clr->accent.Value.y;
									menu_accent[2] = clr->accent.Value.z;

									contrast_one[0] = clr->window.background_one.Value.x;
									contrast_one[1] = clr->window.background_one.Value.y;
									contrast_one[2] = clr->window.background_one.Value.z;

									contrast_two[0] = clr->window.background_two.Value.x;
									contrast_two[1] = clr->window.background_two.Value.y;
									contrast_two[2] = clr->window.background_two.Value.z;

									inline_c[0] = clr->window.stroke.Value.x;
									inline_c[1] = clr->window.stroke.Value.y;
									inline_c[2] = clr->window.stroke.Value.z;

									outline_c[0] = clr->widgets.stroke_two.Value.x;
									outline_c[1] = clr->widgets.stroke_two.Value.y;
									outline_c[2] = clr->widgets.stroke_two.Value.z;

									text_active[0] = clr->widgets.text.Value.x;
									text_active[1] = clr->widgets.text.Value.y;
									text_active[2] = clr->widgets.text.Value.z;

									text_inactive[0] = clr->widgets.text_inactive.Value.x;
									text_inactive[1] = clr->widgets.text_inactive.Value.y;
									text_inactive[2] = clr->widgets.text_inactive.Value.z;

									text_outline_c[0] = clr->widgets.text_outline.Value.x;
									text_outline_c[1] = clr->widgets.text_outline.Value.y;
									text_outline_c[2] = clr->widgets.text_outline.Value.z;

									previous_theme = var->gui.theme_index;
								}

								if (gui->label_color_edit("Menu Accent", menu_accent, false))
								{
									clr->accent.Value.x = menu_accent[0];
									clr->accent.Value.y = menu_accent[1];
									clr->accent.Value.z = menu_accent[2];
								}

								if (gui->label_color_edit("Contrast One", contrast_one, false))
								{
									clr->window.background_one.Value.x = contrast_one[0];
									clr->window.background_one.Value.y = contrast_one[1];
									clr->window.background_one.Value.z = contrast_one[2];
								}

								if (gui->label_color_edit("Contrast Two", contrast_two, false))
								{
									clr->window.background_two.Value.x = contrast_two[0];
									clr->window.background_two.Value.y = contrast_two[1];
									clr->window.background_two.Value.z = contrast_two[2];
								}

								if (gui->label_color_edit("Inline", inline_c, false))
								{
									clr->window.stroke.Value.x = inline_c[0];
									clr->window.stroke.Value.y = inline_c[1];
									clr->window.stroke.Value.z = inline_c[2];
								}

								if (gui->label_color_edit("Outline", outline_c, false))
								{
									clr->widgets.stroke_two.Value.x = outline_c[0];
									clr->widgets.stroke_two.Value.y = outline_c[1];
									clr->widgets.stroke_two.Value.z = outline_c[2];
								}


								if (gui->label_color_edit("Text Active", text_active, false))
								{
									clr->widgets.text.Value.x = text_active[0];
									clr->widgets.text.Value.y = text_active[1];
									clr->widgets.text.Value.z = text_active[2];
								}

								if (gui->label_color_edit("Text Inctive", text_inactive, false))
								{
									clr->widgets.text_inactive.Value.x = text_inactive[0];
									clr->widgets.text_inactive.Value.y = text_inactive[1];
									clr->widgets.text_inactive.Value.z = text_inactive[2];
								}

								if (gui->label_color_edit("Text Outline", text_outline_c, false))
								{
									clr->widgets.text_outline.Value.x = text_outline_c[0];
									clr->widgets.text_outline.Value.y = text_outline_c[1];
									clr->widgets.text_outline.Value.z = text_outline_c[2];
								}

								if (gui->label_color_edit("Warning Text Color", warning_c, false))
								{
									clr->widgets.warning.Value.x = warning_c[0];
									clr->widgets.warning.Value.y = warning_c[1];
									clr->widgets.warning.Value.z = warning_c[2];
								}
								gui->dropdown("Preset", &var->gui.theme_index, themes, IM_ARRAYSIZE(themes));
							}

							if (style_subtab == 1)
							{
								const char* performance_modes[] = { "Fast", "Performance mode" };
								gui->dropdown("Performance Mode", &globals::settings::performance_def, performance_modes, IM_ARRAYSIZE(performance_modes));

								gui->checkbox("Streamproof", &globals::settings::streamproof);
								gui->slider_float("DPI Scaling", &globals::settings::dpi_scaling, 0.5f, 2.0f, "%.2fx");

								gui->checkbox("Window Acrylic", &var->window.window_acrylic);
								if (var->window.window_acrylic) {
									gui->slider_float("Acrylic Intensity", &var->window.acrylic_intensity, 0.0f, 1.0f, "%.2f");
								}

								gui->checkbox("Text Outline", &var->window.text_outline);
								if (var->window.text_outline) {
									gui->slider_float("Outline Thickness", &var->window.text_outline_thickness, 0.1f, 3.0f, false, "%.1fpx");
								}

								gui->checkbox("Watermark", &globals::settings::watermark);
							}
							gui->end_child();
						}
						gui->begin_child("Style", 1, 0, ImVec2(0, 180));
						{
							gui->checkbox("Pointer Design", &var->window.pointer_design);
							gui->checkbox("Hover Highlight", &var->window.hover_hightlight);
							gui->checkbox("Window Gradients", &var->window.window_gradients);
							gui->checkbox("Window Glow", &var->window.window_glow);
							gui->checkbox("Lerp Animations", &var->window.lerp_animations);
							gui->checkbox("Text Outline", &var->window.text_outline);
							if (var->window.text_outline) {
								gui->slider_float("Outline Thickness", &var->window.text_outline_thickness, 1.0f, 3.0f, false, "%.1fpx");
							}
							gui->slider_float("Glow Thickness", &var->window.shadow_size, 0.f, 1000.f, false, "%.0f/1000.0");

							gui->checkbox("Clamp Width", &var->window.clamp_width);
							gui->checkbox("Clamp Content", &var->window.clamp_content);
							gui->slider_int("Max Width", &var->window.max_width, 200, 1000, true, "%dpx");

							gui->slider_float("Window Titlebar Height", &var->window.titlebar, 20, 50);
							gui->slider_float("Window Padding X", &var->window.padding.x, 0, 50);
							gui->slider_float("Window Padding Y", &var->window.padding.y, 0, 50);
							gui->slider_float("Window Border Size", &var->window.border_size, 0, 10);
							gui->slider_float("Window Spacing X", &var->window.spacing.x, 0, 50);
							gui->slider_float("Window Spacing Y", &var->window.spacing.y, 0, 50);
							gui->slider_float("Item Spacing X", &elements->content.spacing.x, 0, 10);
							gui->slider_float("Item Spacing Y", &elements->content.spacing.y, 0, 10);

							if (gui->button("Exit Process")) {
								exit(0);
							}



							if (gui->button("Show Console")) {
								ShowWindow(GetConsoleWindow(), SW_SHOW);
							}



							if (gui->button("Hide Console")) {
								ShowWindow(GetConsoleWindow(), SW_HIDE);
							}
						}
						


						gui->end_child();

					}
					gui->end_content();
				}
				gui->end();
			}

			if (var->gui.current_section[6])
			{
				static bool first_open = true;
				if (first_open) {
					config::refresh();
					first_open = false;
				}

				gui->set_next_window_size_constraints(ImVec2(400, 480), GetIO().DisplaySize);
				gui->begin("cloud", nullptr, var->window.flags);
				{
					draw->window_decorations();
					gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 1));
					gui->begin_content();
					{

							gui->begin_child("Configs", 1, 1, ImVec2(0, 220));
							{
								static float last_refresh_time = 0.0f;
								if (GetTime() > last_refresh_time + 5.0f) {
									config::refresh();
									last_refresh_time = (float)GetTime();
								}

								for (const auto& cfg : config::list)
								{
									bool selected = (config::current_config == cfg);
									if (gui->selectable(cfg.c_str(), &selected, ImVec2(0, 18)))
									{
										config::current_config = cfg;
										strcpy_s(config::name_buffer, cfg.c_str());
									}
								}
							}
							gui->end_child();
							gui->begin_child("Settings", 1, 1, ImVec2(0, 220));
							{

							gui->text_field("Config Name", config::name_buffer, 64);
							ImVec2 button_size = ImVec2((GetWindowWidth() - style->ItemSpacing.x * 3) / 2, 25);

							if (gui->button("Create", 1)) {
								if (strlen(config::name_buffer) > 0)
									config::save(config::name_buffer);
							}
							gui->sameline();
							if (gui->button("Delete", 1)) {
								if (strlen(config::name_buffer) > 0)
									config::remove(config::name_buffer);
							}
							if (gui->button("Load", 1)) {
								if (strlen(config::name_buffer) > 0)
									config::load(config::name_buffer);
							}
							gui->sameline();
							if (gui->button("Save", 1)) {
								if (strlen(config::name_buffer) > 0)
									config::save(config::name_buffer);
							}

							if (gui->button("Auto Load", 1)) {
								if (strlen(config::name_buffer) > 0)
									config::set_auto_load(config::name_buffer);
							}

							if (gui->button("Refresh", 1)) {
								config::refresh();
							}

							if (gui->button("Unload", 1)) {
								config::reset();
							}


							}
							gui->end_child();
					}
					gui->end_content();
				}
				gui->end();
			}
		}

		var->window.width = GetCurrentWindow()->ContentSize.x + style->ItemSpacing.x;

		if (IsMouseHoveringRect(pos, pos + size))
			SetWindowFocus();
	}
	gui->end();
	gui->pop_style_var();
}
	}

