#include "ideal_peak.h"
#include "misc.h"
#include "misc_utils.h"
#include "../cheat_manager.h"
#include "../../../globals.h"

void cheats::misc::c_ideal_peak::run(const cache::entity_t& local)
{
    if (!globals::misc::ideal_peak) {
        globals::misc::has_ideal_peak = false;
        return;
    }

    if (!local.root_part.part.address) return;

    bool active = is_key_active(globals::misc::ideal_peak_key, globals::misc::ideal_peak_mode);
    static bool last_active = false;

    if (active) {
        if (!globals::misc::has_ideal_peak) {
            rbx::part_t root = { local.root_part.part.address };
            globals::misc::ideal_peak_pos = root.get_part_position();
            globals::misc::has_ideal_peak = true;
        }
    } else {
        if (globals::misc::has_ideal_peak) {
            rbx::part_t root = { local.root_part.part.address };
            root.set_part_position(globals::misc::ideal_peak_pos);
            globals::misc::has_ideal_peak = false;
        }
    }

    last_active = active;
}

void cheats::misc::ideal_peak(const cache::entity_t& local)
{
    static std::unique_ptr<c_ideal_peak> instance = std::make_unique<c_ideal_peak>();
    instance->run(local);
}

