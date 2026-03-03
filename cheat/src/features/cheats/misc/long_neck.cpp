#include "long_neck.h"
#include "misc.h"
#include "misc_utils.h"
#include "../cheat_manager.h"

void cheats::misc::c_long_neck::run(const cache::entity_t& local)
{
    if (!globals::misc::long_neck) return;
    if (!local.head.part.address || !local.head.primitive) return;

    rbx::vector3_t position = memory->read<rbx::vector3_t>(local.head.primitive + Offsets::BasePart::Position);
    position.y += globals::misc::neck_length;
    memory->write<rbx::vector3_t>(local.head.primitive + Offsets::BasePart::Position, position);
}
void cheats::misc::long_neck(const cache::entity_t& local)
{
    static std::unique_ptr<c_long_neck> instance = std::make_unique<c_long_neck>();
    instance->run(local);
}

