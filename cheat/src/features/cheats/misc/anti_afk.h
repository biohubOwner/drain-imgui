#pragma once
#include "../cheat_base.h"

namespace cheats
{
    namespace misc
    {
        class c_anti_afk : public c_cheat_base
        {
        public:
            c_anti_afk() : c_cheat_base("anti_afk") {}
            void run(const cache::entity_t& local) override;
        };

        void anti_afk(const cache::entity_t& local);
    }
}

