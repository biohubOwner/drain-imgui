#pragma once
#include "../cheat_base.h"

namespace cheats {
    class c_aimbot : public c_cheat_base {
    public:
        c_aimbot() : c_cheat_base("aimbot") {}
        void run(const cache::entity_t& local) override;
    };

    inline std::unique_ptr<c_aimbot> aimbot = std::make_unique<c_aimbot>();
}

