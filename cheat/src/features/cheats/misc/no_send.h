#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"

namespace cheats::misc {
    class c_no_send : public c_cheat_base {
    public:
        c_no_send() : c_cheat_base("No Send") {}
        virtual void run(const cache::entity_t& local) override;
    };
}

