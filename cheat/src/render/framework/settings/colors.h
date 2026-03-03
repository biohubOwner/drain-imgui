#pragma once
#include "imgui.h"

#include <memory>

class c_colors
{
public:
	ImColor accent{ 141, 41, 81 };

	struct
	{
		ImColor background_one{ 26, 10, 19 };
		ImColor background_two{ 39, 19, 27 };
		ImColor stroke{ 39, 19, 27 };
	} window;

	struct
	{
		ImColor stroke_two{ 39, 19, 27 };
		ImColor text{ 200, 200, 200 };
		ImColor text_inactive{ 136, 136, 136 };
		ImColor text_outline{ 0, 0, 0 };
		ImColor warning{ 255, 180, 60 };
	} widgets;
};

inline std::unique_ptr<c_colors> clr = std::make_unique<c_colors>();

