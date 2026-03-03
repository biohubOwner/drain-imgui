#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"

namespace cheats::misc {
    class c_character_manipulation : public c_cheat_base {
    public:
        c_character_manipulation() : c_cheat_base("Character Manipulation") {}
        virtual void run(const cache::entity_t& local) override;
    };
}

