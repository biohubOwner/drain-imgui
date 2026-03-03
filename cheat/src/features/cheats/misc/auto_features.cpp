#include "auto_features.h"
#include "misc.h"
#include "misc_utils.h"
#include "../cheat_manager.h"

void cheats::misc::c_auto_features::run(const cache::entity_t& local)
{
    if (!local.humanoid.address) return;

    if (globals::visuals::auto_jump)
    {

            memory->write<bool>(local.humanoid.address + Offsets::Humanoid::Jump, true);

    }

    if (globals::visuals::auto_rotate)
    {
        memory->write<bool>(local.humanoid.address + Offsets::Humanoid::AutoRotate, true);
    }
}
void cheats::misc::auto_features(const cache::entity_t& local)
{
    static std::unique_ptr<c_auto_features> instance = std::make_unique<c_auto_features>();
    instance->run(local);
}

