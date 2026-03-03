#include "misc_utils.h"
#include "../../../memory/driver_interface.h"

namespace cheats
{
    bool is_key_active(int key, int mode)
    {
        return is_key_active(key, mode, true);
    }

    bool is_key_active(int key, int mode, bool update_state)
    {
        if (key <= 0 || key >= 256) return true;

        static bool toggle_states[256]{};
        static bool last_key_states[256]{};

        bool is_down = driver->get_key_state(key);

        if (mode == 1)
        {
            if (update_state) {
                if (is_down && !last_key_states[key])
                    toggle_states[key] = !toggle_states[key];

                last_key_states[key] = is_down;
            }
            return toggle_states[key];
        }
        else
        {
            return is_down;
        }
    }
}

