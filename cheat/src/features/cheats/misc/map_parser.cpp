#include "map_parser.h"
#include "../../../globals.h"
#include "../../cache/cache.h"
#include <iostream>
#include <algorithm>
#include <vector>

namespace cheats::misc
{
    void c_map_parser::scan()
    {
        std::unique_lock lock(m_mutex);
        m_cached_parts.clear();
        m_cached_parts.reserve(65000);

        rbx::instance_t workspace = globals::get_workspace();
        if (!workspace.address) return;

        std::vector<rbx::instance_t> stack;
        stack.reserve(4096);
        stack.push_back(workspace);

        while (!stack.empty())
        {
            rbx::instance_t current = stack.back();
            stack.pop_back();

            if (!current.address) continue;

            std::string class_name = current.get_class_name();
            
            if (class_name == "Players" || class_name == "Debris" || class_name == "Lighting" || 
                class_name == "Teams" || class_name == "SoundService" || class_name == "StarterGui") 
                continue;

            if (class_name == "Model" || class_name == "Accessory")
            {
                if (current.find_first_child_by_class("Humanoid").address != 0) 
                    continue;
                
                if (class_name == "Accessory") 
                    continue;
            }

            if (class_name == "Part" || class_name == "MeshPart" || class_name == "CornerWedgePart" || 
                class_name == "WedgePart" || class_name == "TrussPart")
            {
                rbx::part_t part(current.address);
                rbx::primitive_t primitive = part.get_primitive();

                if (primitive.address)
                {
                    std::uint8_t flags = memory->read<std::uint8_t>(primitive.address + Offsets::BasePart::PrimitiveFlags);
                    if (flags & Offsets::PrimitiveFlags::CanCollide)
                    {
                        rbx::vector3_t size = primitive.get_size();

                        if (size.x >= 0.5f || size.y >= 0.5f || size.z >= 0.5f)
                        {
                            parsed_part_t parsed;
                            parsed.position = primitive.get_position();
                            parsed.size = size;
                            parsed.rotation = primitive.get_rotation();
                            
                            float max_s = std::max({size.x, size.y, size.z});
                            parsed.radius = max_s * 0.866f; // max_s * sqrt(3)/2
                            parsed.radius_sq = parsed.radius * parsed.radius;

                            m_cached_parts.push_back(parsed);
                        }
                    }
                }
            }

            std::vector<rbx::instance_t> children = current.get_children();
            for (const auto& child : children)
            {
                stack.push_back(child);
            }
        }
    }

    bool c_map_parser::is_visible(const rbx::vector3_t& start, const rbx::vector3_t& end, std::uint64_t ignore_model, bool include_players)
    {
        std::shared_lock lock(m_mutex);

        if (m_cached_parts.empty() && !include_players) return true;

        rbx::vector3_t dir = end - start;
        float max_dist = dir.magnitude();
        if (max_dist < 0.001f) return true;

        rbx::vector3_t unit_dir = dir / max_dist;
        rbx::vector3_t inv_dir = { 1.0f / unit_dir.x, 1.0f / unit_dir.y, 1.0f / unit_dir.z };

        // Ray struct for Slab Method
        struct Ray {
            rbx::vector3_t origin;
            rbx::vector3_t dir;
            rbx::vector3_t inv_dir;
        } ray = { start, unit_dir, inv_dir };

        auto intersects = [&](const rbx::vector3_t& part_pos, const rbx::vector3_t& part_size, const rbx::matrix3_t& part_rot, float radius_sq) -> bool {
            // 1. Bounding Sphere Pre-check
            rbx::vector3_t to_center = part_pos - ray.origin;
            float projection = to_center.dot(ray.dir);

            if (projection < -1.0f) return false;
            if (projection > max_dist + 1.0f) return false;

            float dist_sq = to_center.dot(to_center) - (projection * projection);
            if (dist_sq > radius_sq) return false;

            // 2. OBB Intersection (Slab Method in Local Space)
            rbx::vector3_t local_origin = (ray.origin - part_pos);
            rbx::vector3_t rotated_origin = {
                local_origin.dot({part_rot.data[0], part_rot.data[3], part_rot.data[6]}),
                local_origin.dot({part_rot.data[1], part_rot.data[4], part_rot.data[7]}),
                local_origin.dot({part_rot.data[2], part_rot.data[5], part_rot.data[8]})
            };

            rbx::vector3_t rotated_dir = {
                ray.dir.dot({part_rot.data[0], part_rot.data[3], part_rot.data[6]}),
                ray.dir.dot({part_rot.data[1], part_rot.data[4], part_rot.data[7]}),
                ray.dir.dot({part_rot.data[2], part_rot.data[5], part_rot.data[8]})
            };

            rbx::vector3_t half_size = part_size * 0.5f;
            rbx::vector3_t local_inv_dir = { 1.0f / rotated_dir.x, 1.0f / rotated_dir.y, 1.0f / rotated_dir.z };

            float t1 = (-half_size.x - rotated_origin.x) * local_inv_dir.x;
            float t2 = (half_size.x - rotated_origin.x) * local_inv_dir.x;
            float t3 = (-half_size.y - rotated_origin.y) * local_inv_dir.y;
            float t4 = (half_size.y - rotated_origin.y) * local_inv_dir.y;
            float t5 = (-half_size.z - rotated_origin.z) * local_inv_dir.z;
            float t6 = (half_size.z - rotated_origin.z) * local_inv_dir.z;

            float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
            float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

            if (tmax < 0 || tmin > tmax) return false;
            if (tmin > max_dist) return false;

            return true;
        };

        // Check static map parts
        for (const auto& part : m_cached_parts) {
            if (intersects(part.position, part.size, part.rotation, part.radius_sq))
                return false;
        }

        // Check dynamic player parts
        if (include_players) {
            std::shared_ptr<std::vector<cache::entity_t>> players;
            {
                std::lock_guard<std::mutex> lock(cache::mtx);
                players = cache::cached_players;
            }

            float max_dist_sq = max_dist * max_dist;

            for (const auto& player : *players) {
                if (player.instance.address == ignore_model) continue;
                if (player.health <= 0) continue;

                // Optimization: Directional and Distance check for player
                rbx::vector3_t to_player = player.root_part.position - ray.origin;
                float projection = to_player.dot(ray.dir);
                
                // If player is behind us or further than the target (with buffer), skip
                if (projection < -5.0f || projection > max_dist + 10.0f) continue;

                // Check perpendicular distance to ray to skip players far off the ray path
                float dist_sq = to_player.dot(to_player) - (projection * projection);
                if (dist_sq > 2500.0f) continue; // Skip players more than 50 units away from the ray path

                for (const auto& part_pair : *player.parts) {
                    const auto& part = part_pair.second;
                    // Approximate radius_sq for player parts
                    float radius_sq = (part.size.x * part.size.x + part.size.y * part.size.y + part.size.z * part.size.z) * 0.25f;
                    if (intersects(part.position, part.size, part.rotation, radius_sq))
                        return false;
                }
            }
        }

        return true;
    }

    void c_map_parser::clear()
    {
        std::unique_lock lock(m_mutex);
        m_cached_parts.clear();
    }
}
