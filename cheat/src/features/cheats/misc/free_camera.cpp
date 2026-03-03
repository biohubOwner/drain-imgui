#include "free_camera.h"
#include "misc.h"
#include "misc_utils.h"
#include "../cheat_manager.h"
#include <chrono>

void cheats::misc::c_free_camera::run(const cache::entity_t& local)
{
    if (!globals::misc::free_camera || !is_key_active(globals::misc::free_camera_key, globals::misc::free_camera_mode))
    {
        m_first_frame = true;
        return;
    }

    if (!globals::camera.address) return;

    rbx::matrix3_t cam_rot = memory->read<rbx::matrix3_t>(globals::camera.address + Offsets::Camera::Rotation);
    rbx::vector3_t look_vec = cam_rot.GetForwardVector();
    rbx::vector3_t right_vec = cam_rot.GetRightVector();
    rbx::vector3_t up_vec = cam_rot.GetUpVector();

    if (m_first_frame)
    {
        m_current_pos = memory->read<rbx::vector3_t>(globals::camera.address + Offsets::Camera::Position);
        m_first_frame = false;
    }

    rbx::vector3_t direction(0, 0, 0);

    if (driver->get_key_state('W')) direction -= look_vec;
    if (driver->get_key_state('S')) direction += look_vec;
    if (driver->get_key_state('A')) direction -= right_vec;
    if (driver->get_key_state('D')) direction += right_vec;
    if (driver->get_key_state(VK_SPACE)) direction += up_vec;
    if (driver->get_key_state(VK_LSHIFT)) direction -= up_vec;

    if (direction.magnitude() > 0.0001f)
        direction = direction.normalize();

    float speed = globals::misc::free_camera_speed / 10.f;
    m_current_pos += direction * speed;

    if ((m_current_pos - m_last_written_pos).magnitude() > 0.001f)
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; i++)
        {
            memory->write<rbx::vector3_t>(globals::camera.address + Offsets::Camera::Position, m_current_pos);
            if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() >= 1)
                break;
        }
        m_last_written_pos = m_current_pos;
    }
    memory->write<rbx::vector3_t>(globals::camera.address + Offsets::Camera::Position, m_current_pos);
}
void cheats::misc::free_camera(const cache::entity_t& local)
{
    static std::unique_ptr<c_free_camera> instance = std::make_unique<c_free_camera>();
    instance->run(local);
}

