#pragma once
#include "../cheat_base.h"
#include <chrono>

namespace cheats {
    class c_triggerbot : public c_cheat_base {
    public:
        c_triggerbot() : c_cheat_base("triggerbot") {}
        void run(const cache::entity_t& local) override;

    private:
        std::chrono::steady_clock::time_point last_shot_time;
        std::chrono::steady_clock::time_point last_check_time;
    };

    inline std::unique_ptr<c_triggerbot> triggerbot = std::make_unique<c_triggerbot>();
}

