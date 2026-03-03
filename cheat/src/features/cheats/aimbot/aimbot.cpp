#include "aimbot.h"
#include "../../../globals.h"
#include "../../cache/cache.h"
#include "../misc/misc_utils.h"
#include "../misc/map_parser.h"
#include <cmath>
#include <algorithm>

namespace cheats {
    static std::uint64_t sticky_target_address = 0;
    static float mouse_x_accumulator = 0.0f;
    static float mouse_y_accumulator = 0.0f;

    rbx::vector3_t get_target_position(cache::entity_t& player) {
        std::vector<std::string> target_parts;


        if (globals::aimbot::hitboxes[0]) target_parts.push_back("Head");
        if (globals::aimbot::hitboxes[1]) {
            target_parts.push_back("UpperTorso");
            target_parts.push_back("Torso");
        }
        if (globals::aimbot::hitboxes[2]) {
            target_parts.push_back("LeftUpperArm");
            target_parts.push_back("Left Arm");
        }
        if (globals::aimbot::hitboxes[3]) {
            target_parts.push_back("RightUpperArm");
            target_parts.push_back("Right Arm");
        }
        if (globals::aimbot::hitboxes[4]) {
            target_parts.push_back("LeftUpperLeg");
            target_parts.push_back("Left Leg");
        }
        if (globals::aimbot::hitboxes[5]) {
            target_parts.push_back("RightUpperLeg");
            target_parts.push_back("Right Leg");
        }

        if (target_parts.empty()) {
            return player.head.position;
        }

        std::string part_name = target_parts[0];
        if (globals::aimbot::random_hitbox) {
            part_name = target_parts[rand() % target_parts.size()];
        } else {

            part_name = target_parts[0];
        }

        if (player.parts->count(part_name)) {
            return (*player.parts)[part_name].position;
        }

        return player.head.position;
    }

    rbx::vector3_t apply_prediction(rbx::vector3_t target_pos, cache::entity_t& target) {
        if (!globals::aimbot::prediction::enabled) return target_pos;

        auto camera = globals::get_camera();
        rbx::vector3_t camera_pos = camera.get_position();
        float distance = (target_pos - camera_pos).magnitude();

        rbx::vector3_t velocity = target.velocity;

        float bullet_speed = globals::aimbot::prediction::bullet_speed ? globals::aimbot::prediction::bullet_speed_value : 1000.0f;
        float time_to_hit = distance / bullet_speed;


        if (globals::aimbot::prediction::velocity_prediction) {
            float prediction_multiplier = globals::aimbot::prediction::multiplier_values[0];
            float strength = globals::aimbot::prediction::multiplier_values[1];

            target_pos.x += velocity.x * time_to_hit * prediction_multiplier * strength * globals::aimbot::prediction::x_multiplier;
            target_pos.y += velocity.y * time_to_hit * prediction_multiplier * strength * globals::aimbot::prediction::y_multiplier;
            target_pos.z += velocity.z * time_to_hit * prediction_multiplier * strength * globals::aimbot::prediction::x_multiplier;
        }


        if (globals::aimbot::prediction::bullet_gravity) {
            float gravity = globals::aimbot::prediction::bullet_gravity_value;
            target_pos.y += 0.5f * gravity * std::pow(time_to_hit, 2);
        }

        return target_pos;
    }

    void c_aimbot::run(const cache::entity_t& local) {
        if (!globals::aimbot::enabled) {
            sticky_target_address = 0;
            mouse_x_accumulator = 0.0f;
            mouse_y_accumulator = 0.0f;
            return;
        }

        if (globals::aimbot::keybind > 0 && !is_key_active(globals::aimbot::keybind, globals::aimbot::keybind_mode)) {
            sticky_target_address = 0;
            mouse_x_accumulator = 0.0f;
            mouse_y_accumulator = 0.0f;
            return;
        }

        auto visual_engine = globals::get_visualengine();
        auto dimensions = visual_engine.get_dimensions();
        auto view_matrix = visual_engine.get_viewmatrix();

        POINT cursor_pos;
        GetCursorPos(&cursor_pos);
        ScreenToClient(globals::roblox_window, &cursor_pos);
        rbx::vector2_t cursor_vec = { (float)cursor_pos.x, (float)cursor_pos.y };

        cache::entity_t* target = nullptr;

        std::shared_ptr<std::vector<cache::entity_t>> players_copy;
        {
            std::lock_guard<std::mutex> lock(cache::mtx);
            players_copy = cache::cached_players;
        }


        auto passes_checks = [&](cache::entity_t& p) {
            if (p.health <= 0 && globals::aimbot::aiming_checks[3]) return false;
            if (p.instance.address == local.instance.address) return false;


            if (globals::aimbot::aiming_checks[0]) {
                if (globals::custom_game::is_detected) {
                    if (p.custom_team != 0 && p.custom_team == globals::custom_game::local_team) return false;
                } else {
                    if (p.team_address != 0 && p.team_address == local.team_address) return false;
                }
            }


            if (globals::aimbot::aiming_checks[1]) {
                auto model = p.instance;
                if (model.address) {
                    if (model.find_first_child_by_class("ForceField").address != 0) return false;
                }
            }


            if ((globals::aimbot::aiming_checks[2] || globals::aimbot::wallcheck) && !globals::aimbot::auto_wall) {
                rbx::vector3_t local_pos = globals::get_camera().get_position();
                rbx::vector3_t target_pos = get_target_position(p);
                if (!cheats::misc::c_map_parser::get().is_visible(local_pos, target_pos, p.instance.address)) {
                    return false;
                }
            }

            return true;
        };

        if (globals::aimbot::root_on_void && sticky_target_address != 0) {
            for (auto& player : *players_copy) {
                if (player.instance.address == sticky_target_address) {
                    if (passes_checks(player)) {
                        target = &player;
                    }
                    break;
                }
            }
        }

        if (!target) {
            float best_distance = FLT_MAX;
        float fov_radius = (globals::aimbot::fov / 100.0f) * (dimensions.x / 2.0f);
        if (globals::aimbot::radius > 0) fov_radius = (float)globals::aimbot::radius;


        rbx::vector2_t fov_center = cursor_vec;
        if (globals::aimbot::method == 1) {
            fov_center = { dimensions.x / 2.0f, dimensions.y / 2.0f };
        }

        for (auto& player : *players_copy) {
            if (!passes_checks(player)) continue;

            rbx::vector3_t target_pos = get_target_position(player);
            rbx::vector2_t screen_pos = visual_engine.world_to_screen(target_pos, dimensions, view_matrix);
            if (screen_pos.x == -1 && screen_pos.y == -1) continue;

            float dist = std::sqrt(std::pow(screen_pos.x - fov_center.x, 2) + std::pow(screen_pos.y - fov_center.y, 2));

            if (dist > fov_radius) continue;

            if (globals::aimbot::deadzone) {
                float deadzone_radius = globals::aimbot::deadzone_value * 10.0f;
                if (dist < deadzone_radius) continue;
            }

            if (dist < best_distance) {
                best_distance = dist;
                target = &player;
            }
        }

            if (target) {
                sticky_target_address = target->instance.address;


                rbx::vector3_t target_pos = get_target_position(*target);
                rbx::vector2_t screen_pos = visual_engine.world_to_screen(target_pos, dimensions, view_matrix);
                if (screen_pos.x != -1 && screen_pos.y != -1) {
                    globals::aimbot::target_screen_pos = { screen_pos.x, screen_pos.y };
                    globals::aimbot::has_target = true;
                } else {
                    globals::aimbot::has_target = false;
                }
            } else {
                sticky_target_address = 0;
                mouse_x_accumulator = 0.0f;
                mouse_y_accumulator = 0.0f;
                globals::aimbot::has_target = false;
            }
        }

        if (target) {
            rbx::vector3_t target_pos = get_target_position(*target);
            target_pos = apply_prediction(target_pos, *target);


            rbx::vector2_t screen_pos = visual_engine.world_to_screen(target_pos, dimensions, view_matrix);
            if (screen_pos.x != -1 && screen_pos.y != -1) {
                globals::aimbot::target_screen_pos = { screen_pos.x, screen_pos.y };
                globals::aimbot::has_target = true;
            } else {
                globals::aimbot::has_target = false;
            }

            if (globals::aimbot::method == 0) {
                rbx::vector2_t screen_pos = visual_engine.world_to_screen(target_pos, dimensions, view_matrix);
                if (screen_pos.x != -1 && screen_pos.y != -1) {
                    float smoothing_x = globals::aimbot::smoothing ? globals::aimbot::smoothing_values[0] : 1.0f;
                    float smoothing_y = globals::aimbot::smoothing ? globals::aimbot::smoothing_values[1] : 1.0f;

                    float delta_x = (screen_pos.x - cursor_vec.x);
                    float delta_y = (screen_pos.y - cursor_vec.y);

                    if (globals::aimbot::smoothing) {
                        delta_x /= (smoothing_x / 10.0f + 1.0f);
                        delta_y /= (smoothing_y / 10.0f + 1.0f);
                    }

                    mouse_x_accumulator += delta_x;
                    mouse_y_accumulator += delta_y;

                    int move_x = static_cast<int>(mouse_x_accumulator);
                    int move_y = static_cast<int>(mouse_y_accumulator);

                    mouse_x_accumulator -= move_x;
                    mouse_y_accumulator -= move_y;

                    if (move_x != 0 || move_y != 0) {
                        mouse_event(MOUSEEVENTF_MOVE, move_x, move_y, 0, 0);
                    }
                }
            } else if (globals::aimbot::method == 1) {
                auto camera = globals::get_camera();
                if (camera.address) {
                    rbx::vector3_t camera_pos = camera.get_position();
                    rbx::matrix3_t current_rotation = camera.get_rotation();

                    rbx::vector3_t relative_pos = target_pos - camera_pos;
                    rbx::vector3_t forward = relative_pos.normalize();
                    rbx::vector3_t current_up(current_rotation.data[1], current_rotation.data[4], current_rotation.data[7]);

                    rbx::vector3_t up = current_up;
                    rbx::vector3_t right = up.cross(forward);

                    if (right.magnitudeSquared() < 0.0001f) {
                        up = { 0, 1, 0 };
                        right = up.cross(forward);
                        if (right.magnitudeSquared() < 0.0001f) {
                            up = { 0, 0, 1 };
                            right = up.cross(forward).normalize();
                        } else {
                            right = right.normalize();
                        }
                    } else {
                        right = right.normalize();
                    }
                    up = forward.cross(right).normalize();

                    rbx::matrix3_t target_rotation(
                        right.x, up.x, -forward.x,
                        right.y, up.y, -forward.y,
                        right.z, up.z, -forward.z
                    );

                    float smoothing = globals::aimbot::smoothing ? (globals::aimbot::smoothing_values[0] + globals::aimbot::smoothing_values[1]) / 2.0f : 0.0f;
                    float t = 1.0f;
                    if (globals::aimbot::smoothing && smoothing > 0.0f) {
                        t = 1.0f / (smoothing / 5.0f + 1.0f);
                    }

                    rbx::matrix3_t lerped_rotation;
                    for (int i = 0; i < 9; ++i) {
                        lerped_rotation.data[i] = current_rotation.data[i] + (target_rotation.data[i] - current_rotation.data[i]) * t;
                    }
                    lerped_rotation.orthonormalize();
                    camera.set_rotation(lerped_rotation);
                }
            } else if (globals::aimbot::method == 2) {
                if (globals::aimbot::target_strafe::enabled) {
                    rbx::part_t root = { local.root_part.part.address };
                    if (root.address) {
                        static float orbit_angle = 0.0f;
                        orbit_angle += (globals::aimbot::target_strafe::speed / 100.0f);

                        rbx::vector3_t tp_pos = target_pos;
                        float rad = globals::aimbot::target_strafe::radius;

                        if (rad > 0) {
                            tp_pos.x += std::cos(orbit_angle) * rad;
                            tp_pos.z += std::sin(orbit_angle) * rad;
                        }

                        tp_pos.y += globals::aimbot::target_strafe::height;

                        root.set_part_position(tp_pos);
                    }
                }
            }
        }
    }
}

