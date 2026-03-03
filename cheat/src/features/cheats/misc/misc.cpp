#include "misc.h"
#include <thread>
#include <chrono>
#include <mutex>
#include "../../../manager/manager.h"
#include "../cheat_manager.h"
#include "speed_hack.h"
#include "fly_hack.h"
#include "no_clip.h"
#include "jump_bug.h"
#include "character_manipulation.h"
#include "free_camera.h"
#include "long_neck.h"
#include "ideal_peak.h"
#include "no_send.h"
#include "anti_afk.h"
#include "waypoint.h"
#include "sound_features.h"
#include "ping_spike.h"
#include "auto_features.h"
#include "rage_misc.h"
#include "anti_aim.h"
#include "world_visuals.h"
#include "../aimbot/aimbot.h"
#include "../aimbot/triggerbot.h"

namespace cheats
{
    namespace misc
    {
        void thread_one()
        {
            while (globals::running)
            {
                cache::entity_t local;
                {
                    std::lock_guard<std::mutex> lock(cache::mtx);
                    local = cache::cached_local_player;
                }
                cheat_manager->run_all(local);

                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }

        void aimbot_thread()
        {
            while (globals::running)
            {
                cache::entity_t local;
                {
                    std::lock_guard<std::mutex> lock(cache::mtx);
                    local = cache::cached_local_player;
                }

                if (globals::aimbot::enabled)
                {
                    cheat_manager->run_by_name("aimbot", local);
                    cheat_manager->run_by_name("triggerbot", local);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        void visuals_thread()
        {
            while (globals::running)
            {

                cache::entity_t local;
                {
                    std::lock_guard<std::mutex> lock(cache::mtx);
                    local = cache::cached_local_player;
                }


                cheat_manager->run_by_name("world_visuals", local);

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        void start_threads()
        {
            cheat_manager->register_cheat(std::make_unique<c_speed_hack>());
            cheat_manager->register_cheat(std::make_unique<c_fly_hack>());
            cheat_manager->register_cheat(std::make_unique<c_no_clip>());
            cheat_manager->register_cheat(std::make_unique<c_jump_bug>());
            cheat_manager->register_cheat(std::make_unique<c_character_manipulation>());
            cheat_manager->register_cheat(std::make_unique<c_free_camera>());
            cheat_manager->register_cheat(std::make_unique<c_long_neck>());
            cheat_manager->register_cheat(std::make_unique<c_ideal_peak>());
            cheat_manager->register_cheat(std::make_unique<c_no_send>());
            cheat_manager->register_cheat(std::make_unique<c_anti_afk>());
            cheat_manager->register_cheat(std::make_unique<c_waypoint>());
            cheat_manager->register_cheat(std::make_unique<c_sound_features>());
            cheat_manager->register_cheat(std::make_unique<c_ping_spike>());
            cheat_manager->register_cheat(std::make_unique<c_auto_features>());
            cheat_manager->register_cheat(std::make_unique<c_target_manager>());
            cheat_manager->register_cheat(std::make_unique<c_tickrate_manipulation>());
            cheat_manager->register_cheat(std::make_unique<c_desync>());
            cheat_manager->register_cheat(std::make_unique<c_anti_aim>());
            cheat_manager->register_cheat(std::make_unique<c_world_visuals>());
            cheat_manager->register_cheat(std::move(aimbot));
            cheat_manager->register_cheat(std::move(triggerbot));

            manager::thread_manager::create(thread_one, "misc_thread");
            manager::thread_manager::create(aimbot_thread, "aimbot_thread");
            manager::thread_manager::create(visuals_thread, "visuals_thread");
        }
    }
}

