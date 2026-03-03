#pragma once
#include "../../../globals.h"
#include "../../cache/cache.h"
#include <map>

namespace cheats
{
    bool is_key_active(int key, int mode);
    bool is_key_active(int key, int mode, bool update_state);
}

