#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"

namespace cheats::misc {
    class c_sound_features : public c_cheat_base {
    public:
        c_sound_features() : c_cheat_base("Sound Features") {}
        virtual void run(const cache::entity_t& local) override;
    };
}

