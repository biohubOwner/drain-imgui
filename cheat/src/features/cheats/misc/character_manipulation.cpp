#include "character_manipulation.h"
#include "misc.h"
#include "misc_utils.h"
#include "map_parser.h"
#include "../cheat_manager.h"

void cheats::misc::c_character_manipulation::run(const cache::entity_t& local)
{
    if (!local.humanoid.address) return;

    if (globals::misc::jump_power_enabled)
    {
        if (is_key_active(globals::misc::jump_power_key, globals::misc::jump_power_mode))
            memory->write<float>(local.humanoid.address + Offsets::Humanoid::JumpPower, globals::misc::jump_power_value);
    }

    if (globals::misc::gravity_enabled)
    {
        if (is_key_active(globals::misc::gravity_key, globals::misc::gravity_mode))
        {
            if (globals::workspace.address){
                auto worldinstance = memory->read<std::uint64_t>(globals::workspace.address + Offsets::Workspace::World);
            memory->write<float>(worldinstance + Offsets::Workspace::Gravity, globals::misc::gravity_value);
            }

        }
    }

    if (globals::misc::hip_height_enabled)
    {
        if (is_key_active(globals::misc::hip_height_key, globals::misc::hip_height_mode))
            memory->write<float>(local.humanoid.address + Offsets::Humanoid::HipHeight, globals::misc::hip_height_value);
    }

    if (globals::misc::no_rotate)
    {
        memory->write<bool>(local.humanoid.address + Offsets::Humanoid::AutoRotate, false);
    }
    else
    {
        memory->write<bool>(local.humanoid.address + Offsets::Humanoid::AutoRotate, true);
    }

    if (globals::misc::recoil_reduction)
    {

        static rbx::matrix3_t last_rotation;
        auto camera = globals::get_camera();
        if (camera.address) {
            rbx::matrix3_t current_rotation = camera.get_rotation();

            if (last_rotation.data[0] != 0 || last_rotation.data[1] != 0) {
                float reduction = globals::misc::recoil_reduction_value / 100.0f;
                if (reduction > 0.0f) {
                    rbx::matrix3_t lerped_rotation;
                    for (int i = 0; i < 9; ++i) {
                        lerped_rotation.data[i] = last_rotation.data[i] + (current_rotation.data[i] - last_rotation.data[i]) * (1.0f - reduction);
                    }
                    lerped_rotation.orthonormalize();
                    camera.set_rotation(lerped_rotation);
                    current_rotation = lerped_rotation;
                }
            }
            last_rotation = current_rotation;
        }
    }


    if (globals::misc::no_fall_damage)
    {
        rbx::vector3_t velocity = memory->read<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::AssemblyLinearVelocity);
        if (velocity.y < -50.0f) {
             velocity.y = 0.0f;
             memory->write<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);
        }
    }

    if (globals::misc::spiderman)
    {
        rbx::vector3_t move_direction = memory->read<rbx::vector3_t>(local.humanoid.address + Offsets::Humanoid::MoveDirection);
        if (move_direction.magnitude() > 0.1f)
        {
            rbx::vector3_t head_pos = local.head.position;
            rbx::vector3_t look_ahead = head_pos + (move_direction * 2.0f);
            
            if (!cheats::misc::c_map_parser::get().is_visible(head_pos, look_ahead, 0, false))
            {
                rbx::vector3_t velocity = memory->read<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::AssemblyLinearVelocity);
                velocity.y = 30.0f; // Climb up
                memory->write<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);
            }
        }
    }

    if (globals::misc::click_tp)
    {
        if (is_key_active(globals::misc::click_tp_key, globals::misc::click_tp_mode))
        {
            auto camera = globals::get_camera();
            rbx::vector3_t start = camera.get_position();
            rbx::matrix3_t rot = camera.get_rotation();
            rbx::vector3_t dir = { -rot.data[2], -rot.data[5], -rot.data[8] }; // Forward vector from matrix

            // Simple raycast forward for now, as exact screen-to-world is complex externally
            // We'll find the first hit using map_parser
            float best_t = 1000.0f;
            bool hit = false;
            
            // We need a way to get the intersection point from map_parser.
            // Since c_map_parser doesn't expose it, we'll approximate by checking points along the ray.
            for (float t = 5.0f; t < 500.0f; t += 2.0f)
            {
                rbx::vector3_t point = start + (dir * t);
                if (!cheats::misc::c_map_parser::get().is_visible(start, point, 0, false))
                {
                    memory->write<rbx::vector3_t>(local.root_part.part.address + Offsets::BasePart::Position, point + rbx::vector3_t(0, 5, 0));
                    hit = true;
                    break;
                }
            }
        }
    }

    if (globals::misc::anchor_on_bind)
    {
        if (is_key_active(globals::misc::anchor_key, globals::misc::anchor_mode))
        {
            if (local.root_part.part.address && local.root_part.primitive)
            {
                memory->write<bool>(local.root_part.primitive + Offsets::BasePart::PrimitiveFlags, true);
                memory->write<bool>(local.root_part.primitive + Offsets::PrimitiveFlags::Anchored, true);
            }
        }
        else
        {
            if (local.root_part.part.address && local.root_part.primitive)
            {
                memory->write<bool>(local.root_part.primitive + Offsets::BasePart::PrimitiveFlags, true);
                memory->write<bool>(local.root_part.primitive + Offsets::PrimitiveFlags::Anchored, false);
            }
        }
    }
    if (globals::misc::sit)
    {
        memory->write<bool>(local.humanoid.address + Offsets::Humanoid::Sit, true);
    }
    else
    {
        memory->write<bool>(local.humanoid.address + Offsets::Humanoid::Sit, false);
    }

    if (globals::misc::platform_stand)
    {
        memory->write<bool>(local.humanoid.address + Offsets::Humanoid::PlatformStand, true);
    }
    else
    {
        memory->write<bool>(local.humanoid.address + Offsets::Humanoid::PlatformStand, false);
    }
}
void cheats::misc::character_manipulation(const cache::entity_t& local)
{
    static std::unique_ptr<c_character_manipulation> instance = std::make_unique<c_character_manipulation>();
    instance->run(local);
}

