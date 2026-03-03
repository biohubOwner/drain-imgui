#pragma once
#include "../cheat_base.h"
#include "../../sdk/sdk.h"

namespace cheats::misc {
    class c_tickrate_manipulation : public c_cheat_base {
    public:
        c_tickrate_manipulation() : c_cheat_base("Tickrate Manipulation") {}
        virtual void run(const cache::entity_t& local) override;
    private:
        bool m_initialized = false;
        float get_tickrate();
    };

    class c_desync : public c_cheat_base {
    public:
        c_desync() : c_cheat_base("Desync") {}
        virtual void run(const cache::entity_t& local) override;
    private:
        bool m_is_desyncing = false;
    };

    class c_target_manager : public c_cheat_base {
    public:
        c_target_manager() : c_cheat_base("Target Manager") {}
        virtual void run(const cache::entity_t& local) override;
    };
}

