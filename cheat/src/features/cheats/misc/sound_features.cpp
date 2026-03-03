#include "sound_features.h"
#include "misc.h"
#include "misc_utils.h"
#include "../cheat_manager.h"

void cheats::misc::c_sound_features::run(const cache::entity_t& local)
{
    if (globals::misc::hit_sounds)
    {
        if (is_key_active(globals::misc::hit_sounds_key, globals::misc::hit_sounds_mode))
        {
        }
    }

    if (globals::misc::death_sounds)
    {
        if (is_key_active(globals::misc::death_sounds_key, globals::misc::death_sounds_mode))
        {
        }
    }
}
void cheats::misc::sound_features(const cache::entity_t& local)
{
    static std::unique_ptr<c_sound_features> instance = std::make_unique<c_sound_features>();
    instance->run(local);
}

