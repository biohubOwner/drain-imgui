#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"

namespace cheats::misc {
    class c_ping_spike : public c_cheat_base {
    public:
        c_ping_spike() : c_cheat_base("Ping Spike") {}
        virtual void run(const cache::entity_t& local) override;
    };
}

