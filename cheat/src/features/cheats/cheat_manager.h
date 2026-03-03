#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include "cheat_base.h"

namespace cheats {
    class c_cheat_manager {
    public:
        void register_cheat(std::unique_ptr<c_cheat_base> cheat) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cheats.push_back(std::move(cheat));
        }

        void run_all(const cache::entity_t& local) {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& cheat : m_cheats) {
                if (cheat->get_name() == "aimbot" || cheat->get_name() == "triggerbot")
                    continue;

                cheat->run(local);
            }
        }

        void run_by_name(const std::string& name, const cache::entity_t& local) {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& cheat : m_cheats) {
                if (cheat->get_name() == name) {
                    cheat->run(local);
                    break;
                }
            }
        }

    private:
        std::vector<std::unique_ptr<c_cheat_base>> m_cheats;
        std::mutex m_mutex;
    };

    inline std::unique_ptr<c_cheat_manager> cheat_manager = std::make_unique<c_cheat_manager>();
}

