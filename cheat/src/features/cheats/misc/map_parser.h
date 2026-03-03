#pragma once
#include <vector>
#include <mutex>
#include <shared_mutex>
#include "../../sdk/sdk.h"

namespace cheats::misc
{
    struct parsed_part_t
    {
        rbx::vector3_t position;
        rbx::vector3_t size;
        rbx::matrix3_t rotation;
        float radius;
        float radius_sq;
    };

    class c_map_parser
    {
    public:
        void scan();
        bool is_visible(const rbx::vector3_t& start, const rbx::vector3_t& end, std::uint64_t ignore_model = 0, bool include_players = true);
        void clear();

        const std::vector<parsed_part_t>& get_cached_parts() const { return m_cached_parts; }
        std::shared_mutex& get_mutex() { return m_mutex; }

        static c_map_parser& get()
        {
            static c_map_parser instance;
            return instance;
        }

    private:
        std::vector<parsed_part_t> m_cached_parts;
        std::shared_mutex m_mutex;
    };

    inline void map_parser_scan() { c_map_parser::get().scan(); }
}

