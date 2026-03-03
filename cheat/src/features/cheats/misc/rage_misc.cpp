#include "rage_misc.h"
#include "misc.h"
#include "misc_utils.h"
#include "../cheat_manager.h"
#include <chrono>
#include <cmath>

float cheats::misc::c_tickrate_manipulation::get_tickrate()
{
    auto workspace = globals::get_workspace();
    if (!workspace.address) return 240.0f;
    auto world = memory->read<std::uint64_t>(workspace.address + Offsets::Workspace::World);
    if (!world) return 240.0f;
    return memory->read<float>(world + Offsets::Workspace::WorldStepsPerSecond);
}

void cheats::misc::c_tickrate_manipulation::run(const cache::entity_t& local)
{
    auto workspace = globals::get_workspace();
    if (!m_initialized && workspace.address)
    {
        globals::misc::tickrate_value = get_tickrate();
        m_initialized = true;
    }

    if (!globals::misc::tickrate_manipulation || !workspace.address) return;

    auto world = memory->read<std::uint64_t>(workspace.address + Offsets::Workspace::World);
    if (world)
    {
        memory->write<float>(world + Offsets::Workspace::WorldStepsPerSecond, globals::misc::tickrate_value);
    }
}

void cheats::misc::c_desync::run(const cache::entity_t& local)
{
    if (!globals::misc::desync) return;

    if (is_key_active(globals::misc::desync_key, globals::misc::desync_mode))
    {
        if (!m_is_desyncing)
        {
            auto world = memory->read<std::uint64_t>(globals::workspace.address + Offsets::Workspace::World);
            memory->write<float>(world + Offsets::Workspace::WorldStepsPerSecond, 999999999999999);
            m_is_desyncing = true;
        }
    }
    else
    {
        if (m_is_desyncing)
        {
            if (!m_is_desyncing) return;
           auto world = memory->read<std::uint64_t>(globals::workspace.address + Offsets::Workspace::World);
            memory->write<float>(world + Offsets::Workspace::WorldStepsPerSecond, 240);

            m_is_desyncing = false;
        }
    }
}

void cheats::misc::c_target_manager::run(const cache::entity_t& local)
{
    float closest_dist = FLT_MAX;
    std::uint64_t closest_addr = 0;

    std::lock_guard<std::mutex> lock(cache::mtx);
    for (const auto& player : *cache::cached_players)
    {
        if (player.instance.address == local.instance.address) continue;

        if (player.distance < closest_dist && player.health > 0)
        {
            closest_dist = player.distance;
            closest_addr = player.instance.address;
        }
    }

    globals::misc::target_entity_address = closest_addr;
}
namespace cheats::misc
{
    void tickrate_manipulation()
    {
        static std::unique_ptr<c_tickrate_manipulation> instance = std::make_unique<c_tickrate_manipulation>();
        instance->run(cache::entity_t());
    }

    void desync(const cache::entity_t& local)
    {
        static std::unique_ptr<c_desync> instance = std::make_unique<c_desync>();
        instance->run(local);
    }

    void update_target(const cache::entity_t& local)
    {
        static std::unique_ptr<c_target_manager> instance = std::make_unique<c_target_manager>();
        instance->run(local);
    }
}

