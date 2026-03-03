#pragma once
#include "../../sdk/sdk.h"

struct visuals
{
	static const rbx::vector3_t corners[8];
};

namespace cheats
{
	void hook_visuals();
}

