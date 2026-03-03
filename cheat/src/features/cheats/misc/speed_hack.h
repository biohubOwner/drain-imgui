#pragma once
#include "../cheat_base.h"
#include "../../../globals.h"

namespace cheats::misc {
    class c_speed_hack : public c_cheat_base {
    public:
        c_speed_hack() : c_cheat_base("Speed Hack") {}

        virtual void run(const cache::entity_t& local) override;

    private:
        bool m_was_enabled = false;
        rbx::vector3_t m_last_velocity = { 0, 0, 0 };
        rbx::vector3_t m_last_position = { 0, 0, 0 };
    };
}

