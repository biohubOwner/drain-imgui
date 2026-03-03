#include "jump_bug.h"
#include "misc.h"
#include "misc_utils.h"
#include "../cheat_manager.h"

void cheats::misc::c_jump_bug::run(const cache::entity_t& local)
{
    if (!local.humanoid.address || !local.root_part.part.address || !local.root_part.primitive) return;

    if (globals::misc::jump_bug && is_key_active(globals::misc::jump_bug_key, globals::misc::jump_bug_mode))
    {
        float floor_material = memory->read<float>(local.humanoid.address + Offsets::Humanoid::FloorMaterial);
        if (floor_material != 0)
        {

                memory->write<bool>(local.humanoid.address + Offsets::Humanoid::Jump, true);

        }
    }

}
void cheats::misc::jump_bug(const cache::entity_t& local)
{
    static std::unique_ptr<c_jump_bug> instance = std::make_unique<c_jump_bug>();
    instance->run(local);
}

