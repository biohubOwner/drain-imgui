#include "anti_aim.h"
#include "misc.h"
#include "misc_utils.h"
#include "../../sdk/math/math.h"
#include "../cheat_manager.h"

void cheats::misc::c_anti_aim::run(const cache::entity_t& local)
{
    if (!globals::misc::angles_enabled) return;
    if (!local.root_part.part.address) return;

    if (is_key_active(globals::misc::angles_key, globals::misc::angles_mode))
    {
        if (local.humanoid.address)
            memory->write<bool>(local.humanoid.address + Offsets::Humanoid::AutoRotate, false);

        rbx::vector3_t angles = { 0, 0, 0 };
        if (globals::misc::pitch_base == 1) angles.x = -89.f;
        else if (globals::misc::pitch_base == 2) angles.x = 89.f;
        else if (globals::misc::pitch_base == 3) angles.x = 0.f;
        else angles.x = globals::misc::pitch_value;
        if (globals::misc::yaw_base == 1) angles.y = 180.f;
        else if (globals::misc::yaw_base == 2) angles.y = 90.f;
        else if (globals::misc::yaw_base == 3) angles.y = -90.f;
        else if (globals::misc::yaw_base == 4)
        {
            m_angle += (0.01f * (globals::misc::spin_speed * (rbx::PI / 180.0f)));

            rbx::vector3_t look = { std::sin(m_angle), 0.0f, std::cos(m_angle) };
            rbx::vector3_t up = { 0.0f, 1.0f, 0.0f };

            if (globals::misc::upside_down)
            {
                up.y = -1.f;
                look = { -look.x, -look.y, -look.z };
            }

            rbx::vector3_t right = up.cross(look);
            rbx::matrix3_t rotation;

            rotation.data[0] = right.x; rotation.data[3] = right.y; rotation.data[6] = right.z;
            rotation.data[1] = up.x;    rotation.data[4] = up.y;    rotation.data[7] = up.z;
            rotation.data[2] = look.x;  rotation.data[5] = look.y;  rotation.data[8] = look.z;

            memory->write<rbx::matrix3_t>(local.root_part.part.address + Offsets::BasePart::Rotation, rotation);
            return;
        }
        else angles.y = globals::misc::yaw_value;

        if (globals::misc::yaw_jitter)
        {
            m_jitter = !m_jitter;
            angles.y += m_jitter ? globals::misc::jitter_value : -globals::misc::jitter_value;
        }
        if (globals::misc::roll) angles.z = 180.f;

        rbx::matrix3_t rotation = rbx::matrix3_t::EulerAnglesToMatrix(angles);
        memory->write<rbx::matrix3_t>(local.root_part.primitive + Offsets::BasePart::Rotation, rotation);
    }
}
void cheats::misc::anti_aim(const cache::entity_t& local)
{
    static std::unique_ptr<c_anti_aim> instance = std::make_unique<c_anti_aim>();
    instance->run(local);
}

