#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"

namespace cheats::misc {
    class c_jump_bug : public c_cheat_base {
    public:
        c_jump_bug() : c_cheat_base("Jump Bug") {}
        virtual void run(const cache::entity_t& local) override;
    };
}

