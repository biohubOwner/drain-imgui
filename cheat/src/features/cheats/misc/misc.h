#pragma once
#include "../../../globals.h"
#include "../../cache/cache.h"

namespace cheats
{
    namespace misc
    {
        void speed_hack(const cache::entity_t& local);
        void fly_hack(const cache::entity_t& local);
        void jump_bug(const cache::entity_t& local);
        void no_clip(const cache::entity_t& local);
        void anti_aim(const cache::entity_t& local);
        void world_visuals();
        void character_manipulation(const cache::entity_t& local);
        void free_camera(const cache::entity_t& local);
        void long_neck(const cache::entity_t& local);
        void ideal_peak(const cache::entity_t& local);
        void no_send(const cache::entity_t& local);
        void waypoint(const cache::entity_t& local);
        void sound_features(const cache::entity_t& local);
        void ping_spike(const cache::entity_t& local);
        void auto_features(const cache::entity_t& local);
        void desync(const cache::entity_t& local);
        void tickrate_manipulation();
        float get_tickrate();
        void update_target(const cache::entity_t& local);
        void thread_one();
        void thread_two();

        void start_threads();
    }
}

