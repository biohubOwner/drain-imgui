#pragma once
#include "../../cache/cache.h"
#include <string>

namespace cheats {
    class c_cheat_base {
    public:
        c_cheat_base(const std::string& name) : m_name(name) {}
        virtual ~c_cheat_base() = default;

        virtual void run(const cache::entity_t& local) = 0;

        const std::string& get_name() const { return m_name; }
        bool is_enabled() const { return m_enabled; }
        void set_enabled(bool enabled) { m_enabled = enabled; }

    protected:
        std::string m_name;
        bool m_enabled = false;
    };
}

