#include "triggerbot.h"
#include "../../../globals.h"
#include "../../cache/cache.h"
#include "../misc/misc_utils.h"
#include <cmath>

namespace cheats {
    void c_triggerbot::run(const cache::entity_t& local) {
        if (!globals::aimbot::trigger::enabled) return;

        if (globals::aimbot::trigger::keybind_mode != 0) {
             if (!is_key_active(globals::aimbot::trigger::keybind, globals::aimbot::trigger::keybind_mode))
                 return;
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_shot_time).count() < globals::aimbot::trigger::interval)
            return;

        auto visual_engine = globals::get_visualengine();
        auto dimensions = visual_engine.get_dimensions();
        auto view_matrix = visual_engine.get_viewmatrix();

        rbx::vector2_t center = { dimensions.x / 2.0f, dimensions.y / 2.0f };

        std::shared_ptr<std::vector<cache::entity_t>> players_copy;
        {
            std::lock_guard<std::mutex> lock(cache::mtx);
            players_copy = cache::cached_players;
        }

        bool found_target = false;
        for (auto& player : *players_copy) {
            if (player.health <= 0) continue;
            if (player.instance.address == local.instance.address) continue;
            
            if (globals::settings::teamcheck) {
                if (globals::custom_game::is_detected) {
                    if (player.custom_team != 0 && player.custom_team == globals::custom_game::local_team) continue;
                } else {
                    if (player.team_address != 0 && player.team_address == local.team_address) continue;
                }
            }


            std::vector<std::string> parts_to_check;

            if (globals::aimbot::trigger::hitboxes[0]) parts_to_check.push_back("Head");
            if (globals::aimbot::trigger::hitboxes[1]) {
                parts_to_check.push_back("UpperTorso");
                parts_to_check.push_back("LowerTorso");
                parts_to_check.push_back("Torso");
            }
            if (globals::aimbot::trigger::hitboxes[2]) {
                parts_to_check.push_back("LeftUpperArm");
                parts_to_check.push_back("RightUpperArm");
                parts_to_check.push_back("LeftLowerArm");
                parts_to_check.push_back("RightLowerArm");
                parts_to_check.push_back("Left Arm");
                parts_to_check.push_back("Right Arm");
            }
            if (globals::aimbot::trigger::hitboxes[3]) {
                parts_to_check.push_back("LeftUpperLeg");
                parts_to_check.push_back("RightUpperLeg");
                parts_to_check.push_back("LeftLowerLeg");
                parts_to_check.push_back("RightLowerLeg");
                parts_to_check.push_back("Left Leg");
                parts_to_check.push_back("Right Leg");
            }

            for (const auto& part_name : parts_to_check) {
                if (player.parts->count(part_name)) {
                    rbx::vector3_t pos = (*player.parts)[part_name].position;
                    rbx::vector2_t screen_pos = visual_engine.world_to_screen(pos, dimensions, view_matrix);

                    if (screen_pos.x == -1 && screen_pos.y == -1) continue;

                    float dist = std::sqrt(std::pow(screen_pos.x - center.x, 2) + std::pow(screen_pos.y - center.y, 2));


                    if (dist < (globals::aimbot::trigger::hitbox_scale * 5.0f)) {
                        found_target = true;
                        break;
                    }
                }
            }
            if (found_target) break;
        }

        if (found_target) {

            if (globals::aimbot::trigger::delay > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds((int)globals::aimbot::trigger::delay));
            }

            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

            last_shot_time = std::chrono::steady_clock::now();
        }
    }
}

