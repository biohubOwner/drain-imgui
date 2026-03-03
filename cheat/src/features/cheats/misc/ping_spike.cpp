#include "ping_spike.h"
#include "misc.h"
#include "misc_utils.h"
#include "../cheat_manager.h"
#include <thread>
#include <chrono>

void cheats::misc::c_ping_spike::run(const cache::entity_t& local)
{
    if (!globals::misc::ping_spike) return;

    if (is_key_active(globals::misc::ping_spike_key, globals::misc::ping_spike_mode))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds((int)globals::misc::ping_spike_value));
    }
}
void cheats::misc::ping_spike(const cache::entity_t& local)
{
    static std::unique_ptr<c_ping_spike> instance = std::make_unique<c_ping_spike>();
    instance->run(local);
}

