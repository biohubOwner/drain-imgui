#pragma once
#include <string>
#include <vector>
#include "imgui.h"

#include <memory>

class c_variables
{
public:
	struct
	{
		ImGuiWindowFlags main_flags{ ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_Tooltip };
		ImGuiWindowFlags flags{ ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground };
		ImVec2 padding{ 0, 0 };
		ImVec2 spacing{ 4, 4 };
	float shadow_size{ 30 };
		float shadow_alpha{ 0.3f };
		float border_size{ 0 };
		float rounding{ 4 };
		float width{ 0 };
		float titlebar{ 20 };
		float scrollbar_size{ 2 };
		bool hover_hightlight{ true };
		bool pointer_design{ false };
		bool window_gradients{ true };
		bool window_glow{ true };
		bool window_acrylic{ false };
		int acrylic_mode{ 0 };
		float acrylic_intensity{ 1.0f };
		bool text_outline{ true };
		float text_outline_thickness{ 1.0f };
		bool lerp_animations{ false };
		bool clamp_width{ false };
		bool clamp_content{ false };
		int max_width{ 600 };
	} window;

	struct
	{
		bool current_section[7]{ true, false, false, false, false, false, false };
		const char* section_icons[IM_ARRAYSIZE(current_section)] = {"A", "B", "C", "D", "E", "F", "G"};

		float menu_alpha{ 0 };
		bool menu_opened{ true };
		int menu_key{ 45 };
		int theme_index{ 11 };
	} gui;

	ImVec2 watermark_pos_one{ 0, 0 };
	ImVec2 watermark_pos_two{ 0, 0 };
	ImVec2 keybind_list_pos{ 0, 0 };
	bool keybind_list_enabled{ true };

	struct
	{
		ImFont* icons[2];
		ImFont* tahoma;
		ImFont* visitor;
		ImFont* fontnew;
	} font;
};

inline std::unique_ptr<c_variables> var = std::make_unique<c_variables>();

