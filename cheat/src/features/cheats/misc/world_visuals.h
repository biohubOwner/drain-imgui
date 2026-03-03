#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"
#include <chrono>

namespace cheats::misc {
    class c_world_visuals : public c_cheat_base {
    public:
        c_world_visuals() : c_cheat_base("World Visuals") {}
        virtual void run(const cache::entity_t& local) override;

    private:
        rbx::instance_t m_cached_sky{};
        rbx::instance_t m_cached_atmosphere{};
        rbx::instance_t m_cached_bloom{};
        rbx::instance_t m_cached_sunrays{};
        rbx::instance_t m_cached_color_correction{};
        rbx::instance_t m_cached_depth_of_field{};
        std::chrono::steady_clock::time_point m_last_sky_check = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point m_last_lighting_check = std::chrono::steady_clock::now();
    };
    void world_visuals();
}

