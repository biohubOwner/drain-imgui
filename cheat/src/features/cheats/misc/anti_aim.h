#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"

namespace cheats::misc {
    class c_anti_aim : public c_cheat_base {
    public:
        c_anti_aim() : c_cheat_base("Anti-Aim") {}
        virtual void run(const cache::entity_t& local) override;

    private:
        float m_angle = 0.0f;
        bool m_jitter = false;
    };
    void anti_aim(const cache::entity_t& local);
}

