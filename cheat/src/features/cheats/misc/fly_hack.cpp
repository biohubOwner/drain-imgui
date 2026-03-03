#include "fly_hack.h"
#include "misc.h"
#include "misc_utils.h"
#include "../../../memory/driver_interface.h"
#include "../cheat_manager.h"
#include <chrono>

void cheats::misc::c_fly_hack::run(const cache::entity_t& local)
{
    if (globals::misc::fly_swim && local.humanoid.address)
    {
        memory->write<std::uint8_t>(local.humanoid.address + Offsets::Humanoid::State, 4); // Swimming state
    }

    if (!globals::misc::fly_hack) return;
    if (!local.humanoid.address || !local.root_part.part.address || !local.root_part.primitive) return;

    if (is_key_active(globals::misc::fly_key, globals::misc::fly_mode))
    {
        if (globals::workspace.address)
            memory->write<float>(globals::workspace.address + Offsets::Workspace::Gravity, 0.f);

        std::uint64_t target_primitive = local.root_part.primitive;
        std::uint64_t target_part = local.root_part.part.address;

        if (globals::misc::vehicle_fly)
        {
            // Try to find vehicle part
            // In many games, the player is welded to the vehicle seat
            // We can check if the root part is part of a vehicle model
            auto parent = memory->read<std::uint64_t>(local.root_part.part.address + Offsets::Instance::Parent);
            if (parent)
            {
                std::string parent_name = rbx::nameable_t(parent).get_name();
                // If parent is not the character, it might be a vehicle
                // This is a simple heuristic
                auto character = memory->read<std::uint64_t>(local.instance.address + Offsets::Player::ModelInstance);
                if (parent != character)
                {
                    rbx::part_t p(parent);
                    auto prim = p.get_primitive();
                    if (prim.address)
                    {
                        target_primitive = prim.address;
                        target_part = parent;
                    }
                }
            }
        }

        if (globals::misc::fly_method == 0)
        {
            rbx::vector3_t velocity = { 0, 0, 0 };
            if ((velocity - m_last_velocity).magnitude() > 0.001f)
            {
                auto start = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < 1000; i++)
                {
                    memory->write<rbx::vector3_t>(target_primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1)
                        break;
                }
                m_last_velocity = velocity;
            }
            memory->write<rbx::vector3_t>(target_primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);

            rbx::vector3_t position = memory->read<rbx::vector3_t>(target_primitive + Offsets::BasePart::Position);
            rbx::vector3_t move_direction = memory->read<rbx::vector3_t>(local.humanoid.address + Offsets::Humanoid::MoveDirection);

            position.x += move_direction.x * (globals::misc::fly_speed / 50.f);
            position.z += move_direction.z * (globals::misc::fly_speed / 50.f);

            if (driver->get_key_state(VK_SPACE)) position.y += (globals::misc::fly_speed / 50.f);
            if (driver->get_key_state(VK_LSHIFT)) position.y -= (globals::misc::fly_speed / 50.f);

            if ((position - m_last_position).magnitude() > 0.001f)
            {
                auto start = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < 1000; i++)
                {
                    memory->write<rbx::vector3_t>(target_primitive + Offsets::BasePart::Position, position);
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1)
                        break;
                }
                m_last_position = position;
            }
            memory->write<rbx::vector3_t>(target_primitive + Offsets::BasePart::Position, position);
        }
        else if (globals::misc::fly_method == 1)
        {
            rbx::matrix3_t cam_rot = memory->read<rbx::matrix3_t>(globals::camera.address + Offsets::Camera::Rotation);
            rbx::vector3_t look_vec = cam_rot.GetForwardVector();
            rbx::vector3_t right_vec = cam_rot.GetRightVector();
            rbx::vector3_t up_vec = cam_rot.GetUpVector();

            rbx::vector3_t direction = { 0, 0, 0 };

            if (driver->get_key_state('W')) direction -= look_vec;
            if (driver->get_key_state('S')) direction += look_vec;
            if (driver->get_key_state('A')) direction -= right_vec;
            if (driver->get_key_state('D')) direction += right_vec;
            if (driver->get_key_state(VK_SPACE)) direction += up_vec;
            if (driver->get_key_state(VK_LSHIFT)) direction -= up_vec;

            if (direction.magnitude() > 0.0001f)
                direction = direction.normalize();

            rbx::vector3_t velocity = direction * globals::misc::fly_speed;
            if ((velocity - m_last_velocity).magnitude() > 0.001f)
            {
                auto start = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < 1000; i++)
                {
                    memory->write<rbx::vector3_t>(target_primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1)
                        break;
                }
                m_last_velocity = velocity;
            }
            memory->write<rbx::vector3_t>(target_primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);
        }
        else if (globals::misc::fly_method == 2)
        {
            rbx::vector3_t velocity = { 0, 0, 0 };
            if ((velocity - m_last_velocity).magnitude() > 0.001f)
            {
                auto start = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < 1000; i++)
                {
                    memory->write<rbx::vector3_t>(target_primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1)
                        break;
                }
                m_last_velocity = velocity;
            }
            memory->write<rbx::vector3_t>(target_primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);

            rbx::vector3_t position = memory->read<rbx::vector3_t>(target_primitive + Offsets::BasePart::Position);

            rbx::matrix3_t cam_rot = memory->read<rbx::matrix3_t>(globals::camera.address + Offsets::Camera::Rotation);
            rbx::vector3_t look_vec = cam_rot.GetForwardVector();
            rbx::vector3_t right_vec = cam_rot.GetRightVector();
            rbx::vector3_t up_vec = cam_rot.GetUpVector();

            rbx::vector3_t direction = { 0, 0, 0 };

            if (driver->get_key_state('W')) direction -= look_vec;
            if (driver->get_key_state('S')) direction += look_vec;
            if (driver->get_key_state('A')) direction -= right_vec;
            if (driver->get_key_state('D')) direction += right_vec;
            if (driver->get_key_state(VK_SPACE)) direction += up_vec;
            if (driver->get_key_state(VK_LSHIFT)) direction -= up_vec;

            if (direction.magnitude() > 0.0001f)
                direction = direction.normalize();

            position += direction * (globals::misc::fly_speed / 50.f);

            if ((position - m_last_position).magnitude() > 0.001f)
            {
                auto start = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < 1000; i++)
                {
                    memory->write<rbx::vector3_t>(target_primitive + Offsets::BasePart::Position, position);
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1)
                        break;
                }
                m_last_position = position;
            }
            memory->write<rbx::vector3_t>(target_primitive + Offsets::BasePart::Position, position);
        }
    }
    else
    {
        if (!globals::misc::gravity_enabled && globals::workspace.address)
            memory->write<float>(globals::workspace.address + Offsets::Workspace::Gravity, globals::visuals::world_gravity);
    }
}
void cheats::misc::fly_hack(const cache::entity_t& local)
{
    static std::unique_ptr<c_fly_hack> instance = std::make_unique<c_fly_hack>();
    instance->run(local);
}

