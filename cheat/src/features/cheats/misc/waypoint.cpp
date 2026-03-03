#include "waypoint.h"
#include "misc.h"
#include "misc_utils.h"
#include "../cheat_manager.h"
#include <chrono>

void cheats::misc::c_waypoint::run(const cache::entity_t& local)
{
    if (!local.root_part.part.address) return;


    for (auto& wp : globals::misc::waypoints)
    {
        if (wp.keybind != 0 && is_key_active(wp.keybind, 0))
        {
            rbx::vector3_t pos = wp.pos;
            memory->write<rbx::vector3_t>(local.root_part.part.address + Offsets::BasePart::Position, pos);
        }
    }


    if (globals::misc::waypoint_on_bind)
    {
        if (is_key_active(globals::misc::waypoint_key, globals::misc::waypoint_mode))
        {
            if (!m_has_saved_pos)
            {
                m_saved_position = memory->read<rbx::vector3_t>(local.root_part.part.address + Offsets::BasePart::Position);
                m_has_saved_pos = true;
            }
            else
            {
                memory->write<rbx::vector3_t>(local.root_part.part.address + Offsets::BasePart::Position, m_saved_position);
            }
        }
        else
        {
            if (globals::misc::waypoint_mode == 0)
                m_has_saved_pos = false;
        }
    }

    if (globals::misc::waypoint_on_respawn)
    {
        float health = memory->read<float>(local.humanoid.address + Offsets::Humanoid::Health);

        if (health <= 0)
        {
            if (!m_was_dead)
            {
                m_saved_position = memory->read<rbx::vector3_t>(local.root_part.part.address + Offsets::BasePart::Position);
                m_has_saved_pos = true;
                m_was_dead = true;
            }
        }
        else
        {
            if (m_was_dead && m_has_saved_pos)
            {
                memory->write<rbx::vector3_t>(local.root_part.part.address + Offsets::BasePart::Position, m_saved_position);
                m_was_dead = false;
            }
        }
    }
}

void cheats::misc::waypoint(const cache::entity_t& local)
{
    static std::unique_ptr<c_waypoint> instance = std::make_unique<c_waypoint>();
    instance->run(local);
}

