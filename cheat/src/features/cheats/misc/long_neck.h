#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"

namespace cheats::misc {
    class c_long_neck : public c_cheat_base {
    public:
        c_long_neck() : c_cheat_base("Long Neck") {}
        virtual void run(const cache::entity_t& local) override;
    };
}

