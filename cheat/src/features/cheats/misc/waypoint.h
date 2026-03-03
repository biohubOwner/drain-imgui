#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"

namespace cheats::misc {
    class c_waypoint : public c_cheat_base {
    public:
        c_waypoint() : c_cheat_base("Waypoint") {}
        virtual void run(const cache::entity_t& local) override;

    private:
        rbx::vector3_t m_saved_position = { 0, 0, 0 };
        bool m_has_saved_pos = false;
        rbx::vector3_t m_last_written_pos = { 0, 0, 0 };
        bool m_was_dead = false;
    };
}

