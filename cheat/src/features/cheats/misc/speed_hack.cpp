#include "speed_hack.h"
#include "misc.h"
#include "misc_utils.h"
#include <chrono>

namespace cheats::misc {
    void speed_hack(const cache::entity_t& local)
    {
        static auto instance = std::make_unique<c_speed_hack>();
        instance->run(local);
    }
}

void cheats::misc::c_speed_hack::run(const cache::entity_t& local)
{
    bool active = globals::misc::speed_hack && is_key_active(globals::misc::speed_key, globals::misc::speed_mode);

    if (!active)
    {
        if (m_was_enabled)
        {
            if (local.humanoid.address)
            {
                memory->write<float>(local.humanoid.address + Offsets::Humanoid::WalkspeedCheck, 16.0f);
                memory->write<float>(local.humanoid.address + Offsets::Humanoid::Walkspeed, 16.0f);
            }
            m_was_enabled = false;
        }
        return;
    }

    if (!local.humanoid.address || !local.root_part.part.address || !local.root_part.primitive) return;

    m_was_enabled = true;

    if (globals::misc::speed_method == 0)
    {
        memory->write<float>(local.humanoid.address + Offsets::Humanoid::WalkspeedCheck, globals::misc::speed_value);
        memory->write<float>(local.humanoid.address + Offsets::Humanoid::Walkspeed, globals::misc::speed_value);
    }
    else if (globals::misc::speed_method == 1)
    {
        rbx::vector3_t move_direction = memory->read<rbx::vector3_t>(local.humanoid.address + Offsets::Humanoid::MoveDirection);
        if (move_direction.x != 0 || move_direction.z != 0)
        {
            rbx::vector3_t velocity = move_direction * globals::misc::speed_value;
            velocity.y = memory->read<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::AssemblyLinearVelocity).y;

            if ((velocity - m_last_velocity).magnitude() > 0.001f)
            {
                auto start = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < 1000; i++)
                {
                    memory->write<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1)
                        break;
                }
                m_last_velocity = velocity;
            }
            memory->write<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);
        }
    }
    else if (globals::misc::speed_method == 2)
    {
        rbx::vector3_t move_direction = memory->read<rbx::vector3_t>(local.humanoid.address + Offsets::Humanoid::MoveDirection);
        if (move_direction.x != 0 || move_direction.z != 0)
        {
            rbx::vector3_t position = memory->read<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::Position);
            position.x += move_direction.x * (globals::misc::speed_value / 50.f);
            position.z += move_direction.z * (globals::misc::speed_value / 50.f);

            if ((position - m_last_position).magnitude() > 0.001f)
            {
                auto start = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < 1000; i++)
                {
                    memory->write<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::Position, position);
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1)
                        break;
                }
                m_last_position = position;
            }
            memory->write<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::Position, position);
        }
    }
    else if (globals::misc::speed_method == 3)
    {
        rbx::matrix3_t cam_rot = memory->read<rbx::matrix3_t>(globals::camera.address + Offsets::Camera::Rotation);
        rbx::vector3_t look_vec = cam_rot.GetForwardVector();
        rbx::vector3_t right_vec = cam_rot.GetRightVector();

        rbx::vector3_t direction(0, 0, 0);
        if (driver->get_key_state('W')) direction -= look_vec;
        if (driver->get_key_state('S')) direction += look_vec;
        if (driver->get_key_state('A')) direction -= right_vec;
        if (driver->get_key_state('D')) direction += right_vec;

        direction.y = 0;
        if (direction.magnitude() > 0.0001f)
            direction = direction.normalize();

        rbx::vector3_t velocity = direction * globals::misc::speed_value;
        velocity.y = memory->read<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::AssemblyLinearVelocity).y;

        if ((velocity - m_last_velocity).magnitude() > 0.001f)
        {
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < 1000; i++)
            {
                memory->write<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);
                if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1)
                    break;
            }
            m_last_velocity = velocity;
        }
        memory->write<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::AssemblyLinearVelocity, velocity);
    }
    else if (globals::misc::speed_method == 4)
    {
        rbx::matrix3_t cam_rot = memory->read<rbx::matrix3_t>(globals::camera.address + Offsets::Camera::Rotation);
        rbx::vector3_t look_vec = cam_rot.GetForwardVector();
        rbx::vector3_t right_vec = cam_rot.GetRightVector();

        rbx::vector3_t direction(0, 0, 0);
        if (driver->get_key_state('W')) direction -= look_vec;
        if (driver->get_key_state('S')) direction += look_vec;
        if (driver->get_key_state('A')) direction -= right_vec;
        if (driver->get_key_state('D')) direction += right_vec;

        direction.y = 0;
        if (direction.magnitude() > 0.0001f)
            direction = direction.normalize();

        rbx::vector3_t position = memory->read<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::Position);
        position += direction * (globals::misc::speed_value / 50.f);

        if ((position - m_last_position).magnitude() > 0.001f)
        {
            auto start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < 1000; i++)
            {
                memory->write<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::Position, position);
                if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1)
                    break;
            }
            m_last_position = position;
        }
        memory->write<rbx::vector3_t>(local.root_part.primitive + Offsets::BasePart::Position, position);
    }
}

