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

#if __has_include("../../../divinity-menu-master/imgui_internal.h")
#include "../../../divinity-menu-master/imgui_internal.h"
#endif

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
						gui->sub_section("Rage", 0, section, 4);
						gui->sub_section("Visuals", 1, section, 4);
						gui->sub_section("Anti-Aim", 2, section, 4);
						gui->sub_section("Misc", 3, section, 4);
					}
					gui->end_group();

					gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + elements->section.height - 1));
					gui->begin_content();
					{
						gui->begin_group();
						{
							gui->begin_child("child 1", 2, 2, ImVec2(0, 0));
							{
								if (section == 0)
								{
									static const char* method_types[] = { "Mouse", "Camera", "Teleport" };
									gui->checkbox("Enabled", &globals::aimbot::enabled, &globals::aimbot::keybind, &globals::aimbot::keybind_mode);
									gui->dropdown("Method", &globals::aimbot::method, method_types, IM_ARRAYSIZE(method_types));
									gui->checkbox("Wallcheck", &globals::aimbot::wallcheck);
									gui->checkbox("Auto Wall", &globals::aimbot::auto_wall);
									gui->checkbox("Smoothing", &globals::aimbot::smoothing);
									gui->slider_float("Smoothing X", &globals::aimbot::smoothing_values[0], 0.0f, 100.0f, false, "%.1f/100.0");
									gui->slider_float("Smoothing Y", &globals::aimbot::smoothing_values[1], 0.0f, 100.0f, false, "%.1f/100.0");
								}
								else if (section == 1)
								{
									gui->checkbox("Master Switch", &globals::visuals::masterswitch);
									gui->checkbox("Show Visuals", &globals::visuals::show_visuals);
									gui->checkbox("Map Debug", &globals::visuals::map_debug);
									gui->checkbox("No Shadows", &globals::visuals::no_shadows, &globals::visuals::no_shadows_key, &globals::visuals::no_shadows_mode);
									if (ImGui::Button("Parse Map", ImVec2(-1, 0)))
										cheats::misc::map_parser_scan();
								}
								else if (section == 2)
								{
									gui->checkbox("Enable Anti-Aim", &globals::misc::angles_enabled, &globals::misc::angles_key, &globals::misc::angles_mode);
									gui->slider_float("Pitch", &globals::misc::pitch_value, -89.0f, 89.0f, false, "%.1f");
									gui->slider_float("Yaw", &globals::misc::yaw_value, -180.0f, 180.0f, false, "%.1f");
									gui->checkbox("Yaw Jitter", &globals::misc::yaw_jitter);
									gui->slider_float("Jitter Value", &globals::misc::jitter_value, 0.0f, 180.0f, false, "%.1f");
								}
								else
								{
									gui->checkbox("Speed Hack", &globals::misc::speed_hack, &globals::misc::speed_key, &globals::misc::speed_mode);
									gui->slider_float("Speed Value", &globals::misc::speed_value, 1.0f, 200.0f, false, "%.1f");
									gui->checkbox("Fly", &globals::misc::fly_hack, &globals::misc::fly_key, &globals::misc::fly_mode);
									gui->slider_float("Fly Speed", &globals::misc::fly_speed, 1.0f, 200.0f, false, "%.1f");
									gui->checkbox("Anti AFK", &globals::misc::anti_afk);
								}
							}
							gui->end_child();

							gui->sameline();

							gui->begin_child("child 2", 0, 2, ImVec2(0, 0));
							{
								if (section == 0)
								{
									static const char* hitbox_labels[] = { "Head", "Torso", "Left Arm", "Right Arm", "Left Leg", "Right Leg" };
									static const char* trigger_modes[] = { "Always", "On Key", "Toggle" };
									gui->multi_dropdown("Hitboxes", globals::aimbot::hitboxes, hitbox_labels, IM_ARRAYSIZE(hitbox_labels));
									gui->slider_float("FOV", &globals::aimbot::fov, 0.0f, 100.0f, false, "%.1f%%");
									gui->slider_int("Radius", &globals::aimbot::radius, 0, 500, false, "%d");
									gui->checkbox("Triggerbot", &globals::aimbot::trigger::enabled, &globals::aimbot::trigger::keybind, &globals::aimbot::trigger::keybind_mode);
									gui->dropdown("Trigger Mode", &globals::aimbot::trigger::keybind_mode, trigger_modes, IM_ARRAYSIZE(trigger_modes));
									gui->slider_float("Trigger Delay", &globals::aimbot::trigger::delay, 0.0f, 1000.0f, false, "%.1fms");
									gui->checkbox("Prediction", &globals::aimbot::prediction::enabled);
									gui->slider_float("Prediction X", &globals::aimbot::prediction::x_multiplier, 0.0f, 20.0f, false, "%.1f");
									gui->slider_float("Prediction Y", &globals::aimbot::prediction::y_multiplier, 0.0f, 20.0f, false, "%.1f");
								}
								else if (section == 1)
								{
									gui->checkbox("Crosshair", &globals::visuals::crosshair::enabled);
									gui->slider_float("Crosshair Size", &globals::visuals::crosshair::size, 1.0f, 20.0f, false, "%.1f");
									gui->checkbox("FOV Circle", &globals::visuals::fov_circle::enabled);
									gui->slider_float("Visual FOV", &globals::visuals::fov, 30.0f, 120.0f, false, "%.1f");
									gui->checkbox("Out of View Arrows", &globals::visuals::enemies.arrows);
								}
								else if (section == 2)
								{
									static const char* yaw_bases[] = { "Camera", "Target", "Spin" };
									gui->dropdown("Yaw Base", &globals::misc::yaw_base, yaw_bases, IM_ARRAYSIZE(yaw_bases));
									gui->slider_float("Spin Speed", &globals::misc::spin_speed, 0.0f, 500.0f, false, "%.1f");
									gui->checkbox("Roll", &globals::misc::roll);
									gui->checkbox("Upside Down", &globals::misc::upside_down);
								}
								else
								{
									gui->checkbox("No Clip", &globals::misc::no_clip, &globals::misc::no_clip_key, &globals::misc::no_clip_mode);
									gui->checkbox("Free Camera", &globals::misc::free_camera, &globals::misc::free_camera_key, &globals::misc::free_camera_mode);
									gui->slider_float("Free Camera Speed", &globals::misc::free_camera_speed, 1.0f, 200.0f, false, "%.1f");
									gui->checkbox("Hit Sounds", &globals::misc::hit_sounds, &globals::misc::hit_sounds_key, &globals::misc::hit_sounds_mode);
									gui->checkbox("Death Sounds", &globals::misc::death_sounds, &globals::misc::death_sounds_key, &globals::misc::death_sounds_mode);
								}
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
