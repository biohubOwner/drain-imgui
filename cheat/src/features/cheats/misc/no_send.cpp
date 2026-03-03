#include "no_send.h"
#include "misc.h"
#include "misc_utils.h"
#include "../cheat_manager.h"

void cheats::misc::c_no_send::run(const cache::entity_t& local)
{
    static bool m_is_no_sending = false;

    if (globals::misc::no_send)
    {
        if (!m_is_no_sending)
        {
            memory->write<int>(memory->get_module_address() + Offsets::Senders::PhysicsSenderMaxBandwidthBps, 0);
            m_is_no_sending = true;
        }
    }
    else
    {
        if (m_is_no_sending)
        {

            bool desync_active = globals::misc::desync && is_key_active(globals::misc::desync_key, globals::misc::desync_mode);
            if (!desync_active)
                memory->write<int>(memory->get_module_address() + Offsets::Senders::PhysicsSenderMaxBandwidthBps, 1000);

            m_is_no_sending = false;
        }
    }
}
void cheats::misc::no_send(const cache::entity_t& local)
{
    static std::unique_ptr<c_no_send> instance = std::make_unique<c_no_send>();
    instance->run(local);
}

