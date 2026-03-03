#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"

namespace cheats::misc {
    class c_ideal_peak : public c_cheat_base {
    public:
        c_ideal_peak() : c_cheat_base("Ideal Peak") {}
        virtual void run(const cache::entity_t& local) override;
    };
}

