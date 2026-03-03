#include "cache.h"
#include <thread>
#include <cmath>
#include <unordered_map>
#include <execution>
#include <functional>
#include "globals.h"
#include "../../output/output.h"
#include "../initalize/initalize.h"

struct transform_data {
    rbx::matrix3_t rotation;
    char pad1[0x48];
    rbx::vector3_t position;
    char pad2[0x72];
    rbx::vector3_t size;
};

void cache::run()
{
    struct cached_entity_info {
        std::uint64_t model_instance_address;
        std::string name;
        std::string display_name;
        std::string tool_name;
        std::unordered_map<std::uint64_t, std::string> part_names;
        std::unordered_map<std::uint64_t, std::uint64_t> part_primitives;
        std::unordered_map<std::uint64_t, std::string> part_mesh_ids;
        std::unordered_map<std::uint64_t, rbx::vector3_t> part_scales;

        void clear() {
            part_names.clear();
            part_primitives.clear();
            part_mesh_ids.clear();
            part_scales.clear();
            tool_name.clear();
        }
    };

    static std::unordered_map<std::uint64_t, cached_entity_info> hierarchy_cache;
    static std::mutex hierarchy_mtx;
    static auto last_hierarchy_refresh = std::chrono::steady_clock::now();

    hierarchy_cache.reserve(100000);

    auto populate_entity = [&](rbx::player_t player, cache::entity_t& entity, bool is_local = false, bool refresh_hierarchy = false, rbx::vector3_t local_pos = {}) {
        entity.instance = { player.address };
        entity.team_address = player.get_team().address;

        if (entity.team_address) {
            entity.team_color = rbx::team_t(entity.team_address).get_team_color();
        }

        if (globals::custom_game::is_detected) {
            rbx::model_instance_t model = player.get_model_instance();
            if (model.address) {
                std::uint64_t parent = memory->read<std::uint64_t>(model.address + Offsets::Instance::Parent);
                
                auto check_team = [&](std::uint64_t addr) {
                    if (addr == globals::custom_game::terrorist_folder.address) return 1;
                    if (addr == globals::custom_game::counter_terrorist_folder.address) return 2;
                    return 0;
                };

                entity.custom_team = check_team(parent);
                
                if (entity.custom_team == 0 && parent != 0) {
                    std::uint64_t grandparent = memory->read<std::uint64_t>(parent + Offsets::Instance::Parent);
                    entity.custom_team = check_team(grandparent);
                }

                if (is_local) {
                    globals::custom_game::local_team = entity.custom_team;
                }
            }
        } else {
            entity.custom_team = 0;
        }

        bool needs_info = false;
        {
            std::lock_guard<std::mutex> h_lock(hierarchy_mtx);
            if (refresh_hierarchy || hierarchy_cache.find(player.address) == hierarchy_cache.end() || hierarchy_cache[player.address].part_names.empty()) {
                needs_info = true;
            }
        }

        if (needs_info) {
            cached_entity_info info;
            info.name = player.get_name();
            info.display_name = player.get_display_name();
            rbx::model_instance_t model_instance = player.get_model_instance();
            info.model_instance_address = model_instance.address;

            if (model_instance.address) {
                rbx::instance_t tool = model_instance.find_first_child_by_class("Tool");
                if (tool.address) info.tool_name = tool.get_name();

                auto children = model_instance.get_children<rbx::part_t>();
                for (auto& part : children) {
                    std::string part_class = part.get_class_name();
                    if (part_class.find("Part") != std::string::npos) {
                        std::string part_name = part.get_name();
                        info.part_names[part.address] = part_name;
                        info.part_primitives[part.address] = part.get_primitive().address;

                        if (part_class == "MeshPart") {
                            info.part_mesh_ids[part.address] = reinterpret_cast<rbx::mesh_part_t*>(&part)->get_mesh_id();
                            info.part_scales[part.address] = { 1.0f, 1.0f, 1.0f };
                        } else {
                            rbx::instance_t mesh = part.find_first_child_by_class("SpecialMesh");
                            if (mesh.address) {
                                info.part_mesh_ids[part.address] = reinterpret_cast<rbx::special_mesh_t*>(&mesh)->get_mesh_id();
                                info.part_scales[part.address] = reinterpret_cast<rbx::special_mesh_t*>(&mesh)->get_scale();
                            }
                        }
                    }
                }
            }
            std::lock_guard<std::mutex> h_lock(hierarchy_mtx);
            hierarchy_cache[player.address] = std::move(info);
        }

        cached_entity_info* info_ptr = nullptr;
        {
            std::lock_guard<std::mutex> h_lock(hierarchy_mtx);
            auto it = hierarchy_cache.find(player.address);
            if (it != hierarchy_cache.end())
                info_ptr = &it->second;
        }

        if (!info_ptr) return false;

        entity.name = info_ptr->name;
        entity.display_name = info_ptr->display_name;
        entity.tool_name = info_ptr->tool_name;

        rbx::model_instance_t model_instance = { info_ptr->model_instance_address };
        if (model_instance.address) {
            bool hrp_found = false;
            rbx::vector3_t hrp_pos{};

            for (auto const& [part_addr, part_name] : info_ptr->part_names)
            {
                std::uint64_t primitive_addr = info_ptr->part_primitives[part_addr];
                if (!primitive_addr) continue;

                rbx::primitive_t primitive = { primitive_addr };

                part_data data = {
                    { part_addr },
                    primitive_addr,
                    part_name,
                    primitive.get_position(),
                    primitive.get_size(),
                    info_ptr->part_scales[part_addr],
                    primitive.get_rotation(),
                    info_ptr->part_mesh_ids[part_addr]
                };

                (*entity.parts)[part_name] = data;

                if (part_name == "HumanoidRootPart")
                {
                    entity.root_part = data;
                    entity.velocity = memory->read<rbx::vector3_t>(data.primitive + Offsets::BasePart::AssemblyLinearVelocity);
                    hrp_found = true;
                    hrp_pos = data.position;
                }
                else if (part_name == "Head")
                {
                    entity.head = data;
                }
            }

            entity.humanoid = { model_instance.find_first_child_by_class("Humanoid").address };
            if (entity.humanoid.address) {
                entity.rig_type = entity.humanoid.get_rig_type();
                entity.health = entity.humanoid.get_health();
                entity.maxhealth = entity.humanoid.get_max_health();
            }

            if (hrp_found)
            {
                entity.distance = is_local ? 0.0f : local_pos.distance(hrp_pos);
            }
        }
        return true;
    };

    while (globals::running)
    {
        auto now = std::chrono::steady_clock::now();
        bool refresh_hierarchy = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_hierarchy_refresh).count() > 2000;
        bool needs_rescan = false;

        if (memory->get_process_id() == 0) {
            needs_rescan = true;
        } else {
            if (!globals::datamodel.address || globals::datamodel.get_class_name() != "DataModel") {
                needs_rescan = true;
            }
        }

        if (needs_rescan) {
            logger->log<e_level::INFO>("Game session lost or changed, rescanning...");
            {
                std::unique_lock lock(globals::globals_mtx);
                globals::datamodel = {};
                globals::players = {};
                globals::local_player = {};
                globals::workspace = {};
                globals::lighting = {};
                globals::camera = {};
                globals::run_service = {};
                globals::visualengine = {};
                globals::renderview = {};
                globals::roblox_window = nullptr;
            }

            if (initialize::runservice()) {
                logger->log<e_level::INFO>("Rescan successful!");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                continue;
            }
        }

        if (!globals::datamodel.address || !globals::players.address || !globals::local_player.address)
        {
            std::uint64_t fake_datamodel{ memory->read<std::uint64_t>(memory->get_module_address() + Offsets::FakeDataModel::Pointer) };
            if (fake_datamodel)
            {
                std::unique_lock lock(globals::globals_mtx);
                globals::datamodel = rbx::instance_t(memory->read<std::uint64_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel));
                if (globals::datamodel.address)
                {
                    globals::players = { globals::datamodel.find_first_child_by_class("Players") };
                    globals::workspace = { globals::datamodel.find_first_child_by_class("Workspace") };
                    globals::lighting = { globals::datamodel.find_first_child_by_class("Lighting") };
                }
            }

            if (globals::players.address)
            {
                std::unique_lock lock(globals::globals_mtx);
                globals::local_player = { memory->read<std::uint64_t>(globals::players.address + Offsets::Player::LocalPlayer) };
            }

            if (globals::workspace.address)
        {
            std::unique_lock lock(globals::globals_mtx);
            globals::camera = { memory->read<std::uint64_t>(globals::workspace.address + Offsets::Workspace::CurrentCamera) };

            // Custom game detection: workspace->characters->terrorist / counter terrorist
            auto characters = globals::workspace.find_first_child("characters");
            if (characters.address) {
                auto terrorist = characters.find_first_child("terrorist");
                auto counter_terrorist = characters.find_first_child("counter terrorist");

                if (terrorist.address && counter_terrorist.address) {
                    globals::custom_game::is_detected = true;
                    globals::custom_game::terrorist_folder = terrorist;
                    globals::custom_game::counter_terrorist_folder = counter_terrorist;
                } else {
                    globals::custom_game::is_detected = false;
                }
            } else {
                globals::custom_game::is_detected = false;
            }
        }

            if (!globals::local_player.address)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
        }

        std::vector<rbx::player_t> players;
        if (globals::players.address)
            players = globals::players.get_children<rbx::player_t>();

        if (globals::workspace.address) {
            std::unique_lock lock(globals::globals_mtx);
            globals::camera = { memory->read<std::uint64_t>(globals::workspace.address + Offsets::Workspace::CurrentCamera) };

            // Custom game detection: ugc -> workspace -> characters -> terrorist / counter terrorist
            // or workspace -> characters -> terrorist / counter terrorist
            
            rbx::instance_t characters_folder{};
            
            // Try DataModel -> ugc -> workspace -> characters
            auto ugc = globals::datamodel.find_first_child("ugc");
            if (ugc.address) {
                auto ugc_workspace = ugc.find_first_child("workspace");
                if (!ugc_workspace.address) ugc_workspace = ugc.find_first_child("Workspace");
                
                if (ugc_workspace.address) {
                    characters_folder = ugc_workspace.find_first_child("characters");
                    if (!characters_folder.address) characters_folder = ugc_workspace.find_first_child("Characters");
                }
            }

            // Fallback: Try Workspace -> ugc -> characters
            if (!characters_folder.address) {
                auto w_ugc = globals::workspace.find_first_child("ugc");
                if (w_ugc.address) {
                    characters_folder = w_ugc.find_first_child("characters");
                    if (!characters_folder.address) characters_folder = w_ugc.find_first_child("Characters");
                }
            }

            // Fallback: Try Workspace -> characters
            if (!characters_folder.address) {
                characters_folder = globals::workspace.find_first_child("characters");
                if (!characters_folder.address) characters_folder = globals::workspace.find_first_child("Characters");
            }

            if (characters_folder.address) {
                auto terrorist = characters_folder.find_first_child("terrorist");
                auto counter_terrorist = characters_folder.find_first_child("counter terrorist");

                // Try capitalized if not found
                if (!terrorist.address) terrorist = characters_folder.find_first_child("Terrorist");
                if (!counter_terrorist.address) counter_terrorist = characters_folder.find_first_child("Counter Terrorist");

                if (terrorist.address && counter_terrorist.address) {
                    globals::custom_game::is_detected = true;
                    globals::custom_game::terrorist_folder = terrorist;
                    globals::custom_game::counter_terrorist_folder = counter_terrorist;
                }
                else {
                    globals::custom_game::is_detected = false;
                }
            }
            else {
                globals::custom_game::is_detected = false;
            }
        }

        rbx::vector3_t local_pos{};
        if (globals::local_player.address)
        {
            auto lp = rbx::player_t(globals::local_player.address);
            auto model = lp.get_model_instance();
            if (model.address) {
                auto hrp_inst = model.find_first_child("HumanoidRootPart");
                if (hrp_inst.address)
                    local_pos = rbx::part_t(hrp_inst.address).get_part_position();
            }
        }

        std::vector<cache::entity_t> temp_entities;
        temp_entities.reserve(players.size());
        std::mutex temp_mtx;

        std::for_each(std::execution::par, players.begin(), players.end(), [&](auto& player)
        {
            if (player.address == 0 || player.address == globals::local_player.address) return;

            cache::entity_t entity{};
            if (populate_entity(player, entity, false, refresh_hierarchy, local_pos)) {
                std::lock_guard<std::mutex> t_lock(temp_mtx);
                temp_entities.push_back(std::move(entity));
            }
        });

        if (refresh_hierarchy) {
            last_hierarchy_refresh = now;
            // More intelligent cleanup: only keep players that are currently in the game
            std::unordered_map<std::uint64_t, cached_entity_info> new_cache;
            new_cache.reserve(players.size() + 1);
            
            std::lock_guard<std::mutex> h_lock(hierarchy_mtx);
            for (auto& p : players) {
                if (hierarchy_cache.count(p.address)) {
                    new_cache[p.address] = std::move(hierarchy_cache[p.address]);
                }
            }
            if (globals::local_player.address && hierarchy_cache.count(globals::local_player.address)) {
                new_cache[globals::local_player.address] = std::move(hierarchy_cache[globals::local_player.address]);
            }
            hierarchy_cache = std::move(new_cache);
        }

        cache::entity_t temp_local{};
        if (globals::local_player.address) {
            auto lp = rbx::player_t(globals::local_player.address);
            local_player_team = lp.get_team().address;
            populate_entity(lp, temp_local, true, refresh_hierarchy, local_pos);
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            cached_players = std::make_shared<std::vector<cache::entity_t>>(std::move(temp_entities));
            cached_local_player = std::move(temp_local);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
