#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"

namespace cheats::misc {
    class c_fly_hack : public c_cheat_base {
    public:
        c_fly_hack() : c_cheat_base("Fly Hack") {}
        virtual void run(const cache::entity_t& local) override;

    private:
        rbx::vector3_t m_last_velocity = { 0, 0, 0 };
        rbx::vector3_t m_last_position = { 0, 0, 0 };
    };
}

