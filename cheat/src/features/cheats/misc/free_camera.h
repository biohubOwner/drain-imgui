#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"

namespace cheats::misc {
    class c_free_camera : public c_cheat_base {
    public:
        c_free_camera() : c_cheat_base("Free Camera") {}
        virtual void run(const cache::entity_t& local) override;

    private:
        bool m_first_frame = true;
        rbx::vector3_t m_current_pos = { 0, 0, 0 };
        rbx::vector3_t m_last_written_pos = { 0, 0, 0 };
    };
}

