#include "no_clip.h"
#include "misc.h"
#include "misc_utils.h"
#include "map_parser.h"
#include "../cheat_manager.h"

void cheats::misc::c_no_clip::run(const cache::entity_t& local)
{
    if (!globals::misc::no_clip) return;
    if (!local.humanoid.address) return;

    if (is_key_active(globals::misc::no_clip_key, globals::misc::no_clip_mode))
    {
        if (globals::misc::no_clip_method == 0) // Normal
        {
            auto handle_no_clip = [&](std::uint64_t primitive_address) {
                if (!primitive_address) return;

                std::uint8_t flags = memory->read<std::uint8_t>(primitive_address + Offsets::BasePart::PrimitiveFlags);
                flags &= ~0x8; // Clear CanCollide bit (0x8)
                memory->write<std::uint8_t>(primitive_address + Offsets::BasePart::PrimitiveFlags, flags);
            };

            handle_no_clip(local.root_part.primitive);
            handle_no_clip(local.head.primitive);
            for (auto& pair : *local.parts)
            {
                handle_no_clip(pair.second.primitive);
            }
        }
        else if (globals::misc::no_clip_method == 1) // Map Parser
        {
            // Use map parser to detect walls and teleport through them
            rbx::vector3_t move_direction = memory->read<rbx::vector3_t>(local.humanoid.address + Offsets::Humanoid::MoveDirection);
            if (move_direction.magnitude() > 0.1f)
            {
                rbx::vector3_t pos = local.root_part.position;
                rbx::vector3_t look_ahead = pos + (move_direction * 3.0f);

                if (!cheats::misc::c_map_parser::get().is_visible(pos, look_ahead, 0, false))
                {
                    // Wall detected, teleport slightly ahead
                    rbx::vector3_t tp_pos = pos + (move_direction * 5.0f);
                    memory->write<rbx::vector3_t>(local.root_part.part.address + Offsets::BasePart::Position, tp_pos);
                }
            }
        }
    }
}
void cheats::misc::no_clip(const cache::entity_t& local)
{
    static std::unique_ptr<c_no_clip> instance = std::make_unique<c_no_clip>();
    instance->run(local);
}

