#include "anti_afk.h"
#include "misc.h"
#include "misc_utils.h"
#include "../cheat_manager.h"

void cheats::misc::c_anti_afk::run(const cache::entity_t& local)
{
    static bool m_is_anti_afk = false;

    if (globals::misc::anti_afk)
    {
        if (!m_is_anti_afk)
        {

            memory->write<bool>(memory->get_module_address() + Offsets::FFlags::DebugDisableTimeoutDisconnect, true);


            memory->write<float>(memory->get_module_address() + Offsets::FFlags::PartyPlayerInactivityTimeoutInSeconds, 86400.0f);

            m_is_anti_afk = true;
        }
    }
    else
    {
        if (m_is_anti_afk)
        {

            memory->write<bool>(memory->get_module_address() + Offsets::FFlags::DebugDisableTimeoutDisconnect, false);
            memory->write<float>(memory->get_module_address() + Offsets::FFlags::PartyPlayerInactivityTimeoutInSeconds, 1200.0f);

            m_is_anti_afk = false;
        }
    }
}

void cheats::misc::anti_afk(const cache::entity_t& local)
{
    static std::unique_ptr<c_anti_afk> instance = std::make_unique<c_anti_afk>();
    instance->run(local);
}

