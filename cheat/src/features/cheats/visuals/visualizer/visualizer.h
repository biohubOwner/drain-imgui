#pragma once
#include <windows.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "../../../cache/cache.h"
#include "../visuals.h"
#include "render/render.h"
#include "globals.h"
#include "mesh_utils.h"
#include <render/framework/settings/variables.h>
#include <render/framework/settings/functions.h>
#include "../clipper2lib/include/clipper2/clipper.h"
#include <algorithm>
#include <vector>
#include <unordered_map>

namespace visualizer
{
	inline ImVec2 get_window_offset() {
		if (!globals::roblox_window) return { 0, 0 };
		POINT p{ 0, 0 };
		ClientToScreen(globals::roblox_window, &p);
		return { (float)p.x, (float)p.y };
	}

	inline bool should_render_outline(int type) {
		if (globals::visuals::options::global_outline_type == 0) return false;
		auto& outlines = globals::visuals::options::render_outlines;
		if (type < 0 || type >= outlines.size()) return false;
		return outlines[type] != 0;
	}

	inline float cross_product_2d(const ImVec2& O, const ImVec2& A, const ImVec2& B) {
		return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
	}

	inline float distance_sq(const ImVec2& A, const ImVec2& B) {
		return (A.x - B.x) * (A.x - B.x) + (A.y - B.y) * (A.y - B.y);
	}

	inline std::vector<ImVec2> convex_hull(std::vector<ImVec2>& points) {
		if (points.size() <= 3) return points;

		auto it = std::min_element(points.begin(), points.end(), [](const ImVec2& a, const ImVec2& b) {
			return (a.y < b.y) || (a.y == b.y && a.x < b.x);
			});

		std::swap(points[0], *it);
		ImVec2 p0 = points[0];

		std::sort(points.begin() + 1, points.end(), [&p0](const ImVec2& a, const ImVec2& b) {
			float cross = cross_product_2d(p0, a, b);
			return (cross > 0) || (cross == 0 && distance_sq(p0, a) < distance_sq(p0, b));
			});

		std::vector<ImVec2> hull;
		hull.push_back(points[0]);
		hull.push_back(points[1]);

		for (size_t i = 2; i < points.size(); i++) {
			while (hull.size() > 1 && cross_product_2d(hull[hull.size() - 2], hull.back(), points[i]) <= 0) {
				hull.pop_back();
			}
			hull.push_back(points[i]);
		}

		return hull;
	}

	static void box_calculations(cache::entity_t& entity, ImVec2& c1, ImVec2& c2, bool& valid, rbx::vector2_t dims = {}, rbx::matrix4_t view = {}, ImVec2 window_offset = {})
	{
		float left = FLT_MAX, top = FLT_MAX;
		float right = -FLT_MAX, bottom = -FLT_MAX;
		valid = false;

		if (entity.instance.address == 0)
			return;

		for (auto& parts : *entity.parts)
		{
			const auto& part = parts.second;

			rbx::vector3_t size = part.size;
			rbx::vector3_t pos = part.position;
			rbx::matrix3_t rot = part.rotation;

			if (size.x == 0 || size.y == 0 || size.z == 0)
				continue;

			for (int i = 0; i < 8; i++)
			{
				const auto& corner = visuals::corners[i];
				rbx::vector3_t world = pos + rot * rbx::vector3_t{
					corner.x * size.x * 0.5f,
					corner.y * size.y * 0.5f,
					corner.z * size.z * 0.5f
				};

				rbx::vector2_t out = globals::visualengine.world_to_screen(world, dims, view);

				if (out.x > 0 && out.y > 0 && out.x < dims.x && out.y < dims.y)
				{
					out.x += window_offset.x;
					out.y += window_offset.y;

					valid = true;
					left = std::min(left, out.x);
					top = std::min(top, out.y);
					right = std::max(right, out.x);
					bottom = std::max(bottom, out.y);
				}
			}
		}

		if (!valid || left >= right || top >= bottom)
			return;

		c1 = ImVec2(left, top);
		c2 = ImVec2(right - left, bottom - top);
	}

	static void static_box_calculations(cache::entity_t& entity, ImVec2& c1, ImVec2& c2, bool& valid, rbx::vector2_t dims = {}, rbx::matrix4_t view = {}, ImVec2 window_offset = {})
	{
		valid = false;
		if (entity.instance.address == 0 || entity.root_part.primitive == 0)
			return;

		rbx::vector3_t root_pos = entity.root_part.position;

		rbx::vector3_t top_world = root_pos + rbx::vector3_t{ 0, 2.5f, 0 };
		rbx::vector3_t bottom_world = root_pos + rbx::vector3_t{ 0, -3.0f, 0 };

		rbx::vector2_t top_screen = globals::visualengine.world_to_screen(top_world, dims, view);
		rbx::vector2_t bottom_screen = globals::visualengine.world_to_screen(bottom_world, dims, view);

		if (top_screen.x != -1.0f && bottom_screen.x != -1.0f)
		{
			float height = std::abs(bottom_screen.y - top_screen.y);
			float width = height * 0.6f;

			c1 = ImVec2(top_screen.x - (width / 2.0f) + window_offset.x, top_screen.y + window_offset.y);
			c2 = ImVec2(width, height);
			valid = true;
		}
	}

	static void draw_obb(const rbx::vector3_t& pos, const rbx::vector3_t& size, const rbx::matrix3_t& rot, ImU32 color, rbx::vector2_t dims, rbx::matrix4_t view, ImVec2 window_offset)
	{
		ImVec2 screen_corners[8];
		bool all_valid = true;

		for (int i = 0; i < 8; i++)
		{
			const auto& corner = visuals::corners[i];
			rbx::vector3_t world = pos + rot * rbx::vector3_t{
				corner.x * size.x * 0.5f,
				corner.y * size.y * 0.5f,
				corner.z * size.z * 0.5f
			};

			rbx::vector2_t out = globals::visualengine.world_to_screen(world, dims, view);
			if (out.x == -1.0f) { all_valid = false; break; }

			screen_corners[i] = ImVec2(out.x + window_offset.x, out.y + window_offset.y);
		}

		if (!all_valid) return;

		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
		auto draw_edge = [&](int i, int j) {
			draw_list->AddLine(screen_corners[i], screen_corners[j], color, 1.0f);
		};

		draw_edge(0, 1); draw_edge(1, 3); draw_edge(3, 2); draw_edge(2, 0);
		draw_edge(4, 5); draw_edge(5, 7); draw_edge(7, 6); draw_edge(6, 4);
		draw_edge(0, 4); draw_edge(1, 5); draw_edge(2, 6); draw_edge(3, 7);
	}

	static void box_3d(cache::entity_t& entity, ImU32 color, bool static_box, rbx::vector2_t dims = {}, rbx::matrix4_t view = {}, ImVec2 window_offset = {})
	{
		if (entity.instance.address == 0 || entity.root_part.primitive == 0)
			return;

		rbx::vector3_t root_pos = entity.root_part.position;
		rbx::matrix3_t root_rot = static_box ? rbx::matrix3_t::identity() : entity.root_part.rotation;
		rbx::vector3_t size = static_box ? rbx::vector3_t{ 3.5f, 5.5f, 3.5f } : entity.root_part.size * 2.5f;

		if (!static_box)
		{
			float min_x = FLT_MAX, min_y = FLT_MAX, min_z = FLT_MAX;
			float max_x = -FLT_MAX, max_y = -FLT_MAX, max_z = -FLT_MAX;

			for (auto& parts : *entity.parts)
			{
				const auto& part = parts.second;
				rbx::vector3_t p_pos = part.position;
				rbx::vector3_t p_size = part.size;
				rbx::vector3_t local_pos = root_rot.inverse() * (p_pos - root_pos);

				min_x = std::min(min_x, local_pos.x - p_size.x * 0.5f);
				min_y = std::min(min_y, local_pos.y - p_size.y * 0.5f);
				min_z = std::min(min_z, local_pos.z - p_size.z * 0.5f);
				max_x = std::max(max_x, local_pos.x + p_size.x * 0.5f);
				max_y = std::max(max_y, local_pos.y + p_size.y * 0.5f);
				max_z = std::max(max_z, local_pos.z + p_size.z * 0.5f);
			}

			root_pos = root_pos + root_rot * rbx::vector3_t{ (min_x + max_x) * 0.5f, (min_y + max_y) * 0.5f, (min_z + max_z) * 0.5f };
			size = { max_x - min_x, max_y - min_y, max_z - min_z };
		}
		else {
			root_pos.y -= 0.25f;
		}

		ImVec2 screen_corners[8];
		bool all_valid = true;

		for (int i = 0; i < 8; i++)
		{
			const auto& corner = visuals::corners[i];
			rbx::vector3_t world = root_pos + root_rot * rbx::vector3_t{
				corner.x * size.x * 0.5f,
				corner.y * size.y * 0.5f,
				corner.z * size.z * 0.5f
			};

			rbx::vector2_t out = globals::visualengine.world_to_screen(world, dims, view);
			if (out.x == -1.0f) { all_valid = false; break; }

			screen_corners[i] = ImVec2(out.x + window_offset.x, out.y + window_offset.y);
		}

		if (!all_valid) return;

		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
		auto draw_edge = [&](int i, int j) {
			draw_list->AddLine(screen_corners[i], screen_corners[j], color, 1.0f);
		};

		draw_edge(0, 1); draw_edge(1, 3); draw_edge(3, 2); draw_edge(2, 0);
		draw_edge(4, 5); draw_edge(5, 7); draw_edge(7, 6); draw_edge(6, 4);
		draw_edge(0, 4); draw_edge(1, 5); draw_edge(2, 6); draw_edge(3, 7);
	}

	inline bool is_inside_triangle(ImVec2 a, ImVec2 b, ImVec2 c, ImVec2 p) {
		float area = 0.5f * (-b.y * c.x + a.y * (-b.x + c.x) + a.x * (b.y - c.y) + b.x * c.y);
		if (std::abs(area) < 1e-4f) return false;
		float s = 1.0f / (2.0f * area) * (a.y * c.x - a.x * c.y + (c.y - a.y) * p.x + (a.x - c.x) * p.y);
		float t = 1.0f / (2.0f * area) * (a.x * b.y - a.y * b.x + (a.y - b.y) * p.x + (b.x - a.x) * p.y);
		return s > 0.001f && t > 0.001f && (1.0f - s - t) > 0.001f;
	}

	inline void triangulate_and_fill(ImDrawList* draw, const Clipper2Lib::PathD& path, ImU32 color) {
		if (path.size() < 3) return;

		std::vector<ImVec2> pts;
		for (const auto& p : path) {
			pts.push_back(ImVec2(std::round((float)p.x), std::round((float)p.y)));
		}

		pts.erase(std::unique(pts.begin(), pts.end(), [](const ImVec2& a, const ImVec2& b) {
			return std::abs(a.x - b.x) < 0.01f && std::abs(a.y - b.y) < 0.01f;
			}), pts.end());

		if (pts.size() < 3) return;

		std::vector<int> indices;
		for (int i = 0; i < (int)pts.size(); ++i) indices.push_back(i);

		double area = 0;
		for (size_t i = 0; i < pts.size(); i++) {
			area += (double)pts[i].x * pts[(i + 1) % pts.size()].y - (double)pts[(i + 1) % pts.size()].x * pts[i].y;
		}
		if (area < 0) std::reverse(indices.begin(), indices.end());
		if (std::abs(area) < 0.001f) return;

		int timeout = 5000;
		while (indices.size() > 3 && timeout-- > 0) {
			bool ear_found = false;
			for (int i = 0; i < (int)indices.size(); ++i) {
				int prev = indices[(i + indices.size() - 1) % indices.size()];
				int curr = indices[i];
				int next = indices[(i + 1) % indices.size()];

				ImVec2 a = pts[prev], b = pts[curr], c = pts[next];

				float cp = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
				if (cp <= 0.0001f) continue;

				bool has_point_inside = false;
				for (int j = 0; j < (int)indices.size(); ++j) {
					int idx = indices[j];
					if (idx == prev || idx == curr || idx == next) continue;
					if (is_inside_triangle(a, b, c, pts[idx])) {
						has_point_inside = true;
						break;
					}
				}

				if (!has_point_inside) {
					draw->AddTriangleFilled(a, b, c, color);
					indices.erase(indices.begin() + i);
					ear_found = true;
					break;
				}
			}
			if (!ear_found) {
				break;
			}
		}
		if (indices.size() == 3) {
			draw->AddTriangleFilled(pts[indices[0]], pts[indices[1]], pts[indices[2]], color);
		}
	}

	static void chams(cache::entity_t& entity, ImU32 color, ImU32 fresnel_color = 0, bool glow = false, ImU32 glow_color = 0, bool is_hit = false, bool flash = false, float flash_speed = 1.0f, ImU32 flash_color = 0, bool fade = false, float fade_speed = 1.0f, rbx::vector2_t dimensions = {}, rbx::matrix4_t viewmatrix = {}, ImVec2 window_offset = {}, ImU32 hit_color = globals::visuals::colors::chams_hit_impact, int texture = 0, float glow_intensity = 100.0f)
	{
		ImDrawList* draw = ImGui::GetBackgroundDrawList();

		static std::unordered_map<std::uintptr_t, float> hit_times;
		static std::unordered_map<std::uintptr_t, float> last_health;

		float current_time = (float)ImGui::GetTime();
		if (last_health.count(entity.instance.address)) {
			if (entity.health < last_health[entity.instance.address]) {
				hit_times[entity.instance.address] = current_time;
			}
		}
		last_health[entity.instance.address] = entity.health;

		bool hit = is_hit || (current_time - hit_times[entity.instance.address]) < 0.25f;

		ImU32 final_color = color;
		if (flash)
		{
			float t = (sinf(current_time * flash_speed * 5.0f) + 1.0f) * 0.5f;
			ImVec4 c1 = ImGui::ColorConvertU32ToFloat4(color);
			ImVec4 c2 = ImGui::ColorConvertU32ToFloat4(flash_color);
			final_color = ImGui::ColorConvertFloat4ToU32(ImLerp(c1, c2, t));
		}

		if (fade)
		{
			float t = (sinf(current_time * fade_speed * 5.0f) + 1.0f) * 0.5f;
			ImVec4 c = ImGui::ColorConvertU32ToFloat4(final_color);
			c.w *= t;
			final_color = ImGui::ColorConvertFloat4ToU32(c);
		}

		if (texture == 1) // Bubble
		{
			ImVec4 c = ImGui::ColorConvertU32ToFloat4(final_color);
			c.w *= 0.3f;
			final_color = ImGui::ColorConvertFloat4ToU32(c);
		}
		else if (texture == 2) // ForceField
		{
			ImVec4 c = ImGui::ColorConvertU32ToFloat4(final_color);
			c.w *= 0.15f;
			final_color = ImGui::ColorConvertFloat4ToU32(c);
		}
		else if (texture == 3) // Neon
		{
			ImVec4 c = ImGui::ColorConvertU32ToFloat4(final_color);
			c.w = 1.0f;
			final_color = ImGui::ColorConvertFloat4ToU32(c);
		}
		else if (texture == 4) // Glass
		{
			ImVec4 c = ImGui::ColorConvertU32ToFloat4(final_color);
			c.w *= 0.1f;
			final_color = ImGui::ColorConvertFloat4ToU32(c);
		}
		else if (texture == 5) // Wood
		{
			ImVec4 c = ImGui::ColorConvertU32ToFloat4(final_color);
			c.x *= 0.6f; c.y *= 0.4f; c.z *= 0.2f; // Brownish
			final_color = ImGui::ColorConvertFloat4ToU32(c);
		}
		else if (texture == 6) // Slate
		{
			ImVec4 c = ImGui::ColorConvertU32ToFloat4(final_color);
			c.x *= 0.4f; c.y *= 0.4f; c.z *= 0.4f; // Greyish
			final_color = ImGui::ColorConvertFloat4ToU32(c);
		}

		Clipper2Lib::PathsD all_paths;

		for (auto& pair : *entity.parts)
		{
			const auto& name = pair.first;
			const auto& data = pair.second;
			if (name == "HumanoidRootPart") continue;

			rbx::vector3_t pos = data.position;
			rbx::vector3_t size = data.size;
			rbx::matrix3_t rot = data.rotation;

			rbx::vector3_t local_corners[8] = {
				{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f},
				{-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}
			};

			std::vector<ImVec2> screen_points;
			for (int i = 0; i < 8; ++i)
			{
				rbx::vector3_t scaled = { local_corners[i].x * size.x, local_corners[i].y * size.y, local_corners[i].z * size.z };
				rbx::vector3_t rotated = {
					scaled.x * rot.data[0] + scaled.y * rot.data[1] + scaled.z * rot.data[2],
					scaled.x * rot.data[3] + scaled.y * rot.data[4] + scaled.z * rot.data[5],
					scaled.x * rot.data[6] + scaled.y * rot.data[7] + scaled.z * rot.data[8]
				};
				rbx::vector3_t world_pos = { rotated.x + pos.x, rotated.y + pos.y, rotated.z + pos.z };

				rbx::vector2_t screen = globals::visualengine.world_to_screen(world_pos, dimensions, viewmatrix);
				if (screen.x != -1 && screen.y != -1)
				{
					screen_points.push_back({ std::round(screen.x + window_offset.x), std::round(screen.y + window_offset.y) });
				}
			}

			if (screen_points.size() >= 3)
			{
				std::vector<ImVec2> hull = convex_hull(screen_points);
				Clipper2Lib::PathD path;
				for (const auto& p : hull)
					path.push_back(Clipper2Lib::PointD((double)p.x, (double)p.y));

				all_paths.push_back(path);
			}
		}

		if (all_paths.empty()) return;

		Clipper2Lib::PathsD union_result = Clipper2Lib::Union(all_paths, Clipper2Lib::FillRule::NonZero);

		ImDrawListFlags old_flags = draw->Flags;

		if (glow)
		{
			for (const auto& path : union_result)
			{
				if (path.size() < 3) continue;

				std::vector<ImVec2> pts;
				pts.reserve(path.size());
				for (const auto& p : path) pts.push_back(ImVec2(std::round((float)p.x), std::round((float)p.y)));

				// Remove duplicate consecutive points
				pts.erase(std::unique(pts.begin(), pts.end(), [](const ImVec2& a, const ImVec2& b) {
					return std::abs(a.x - b.x) < 0.01f && std::abs(a.y - b.y) < 0.01f;
					}), pts.end());

				if (pts.size() < 3) continue;

				float intensity_factor = glow_intensity / 100.0f;
				float current_alpha = (float)((final_color >> 24) & 0xFF) / 255.0f;
				ImU32 alpha_val = (ImU32)(globals::visuals::glow_strength * 255.0f * intensity_factor * current_alpha);
				
				if (alpha_val == 0) continue;
				
				ImU32 final_glow_col = (glow_color != 0 ? glow_color : final_color) & 0x00FFFFFF | (alpha_val << 24);

				::draw->shadow_convex_poly(draw, pts.data(), (int)pts.size(), final_glow_col, 15.0f * intensity_factor, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
				::draw->shadow_convex_poly(draw, pts.data(), (int)pts.size(), final_glow_col, 30.0f * intensity_factor, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
			}
		}

		draw->Flags &= ~(ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines);

		for (const auto& path : union_result)
		{
			if (path.size() < 3) continue;

			std::vector<ImVec2> pts;
			for (const auto& p : path) pts.push_back(ImVec2(std::round((float)p.x), std::round((float)p.y)));

			if (hit && globals::visuals::options::chams_hit_impact)
			{
				triangulate_and_fill(draw, path, hit_color);
				if (!glow)
					draw->AddPolyline((const ImVec2*)pts.data(), (int)pts.size(), hit_color, ImDrawFlags_Closed, globals::visuals::options::chams_outline_thickness);
			}
			else
			{
				triangulate_and_fill(draw, path, final_color);

				if (!glow)
					draw->AddPolyline((const ImVec2*)pts.data(), (int)pts.size(), final_color, ImDrawFlags_Closed, globals::visuals::options::chams_outline_thickness);

				if (texture == 1 || texture == 2 || texture == 4) // Bubble, ForceField, Glass
				{
					ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
					c.w *= (texture == 1 ? 0.6f : (texture == 2 ? 0.8f : 0.4f));
					draw->AddPolyline((const ImVec2*)pts.data(), (int)pts.size(), ImGui::ColorConvertFloat4ToU32(c), ImDrawFlags_Closed, globals::visuals::options::chams_outline_thickness + 1.5f);
				}
				else if (texture == 3) // Neon
				{
					ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
					c.w = 1.0f;
					draw->AddPolyline((const ImVec2*)pts.data(), (int)pts.size(), ImGui::ColorConvertFloat4ToU32(c), ImDrawFlags_Closed, globals::visuals::options::chams_outline_thickness + 2.5f);
				}
			}

			if (should_render_outline(7) && !glow)
			{
				ImU32 f_col = fresnel_color != 0 ? fresnel_color : globals::visuals::colors::chams_fresnel;
				draw->AddPolyline((const ImVec2*)pts.data(), (int)pts.size(), f_col, ImDrawFlags_Closed, globals::visuals::options::chams_outline_thickness);
			}
		}

		draw->Flags = old_flags;
	}

	static void mesh_chams(cache::entity_t& entity, ImU32 color, bool glow = false, ImU32 glow_color = 0, bool fade = false, float fade_speed = 1.0f, rbx::vector2_t dimensions = {}, rbx::matrix4_t viewmatrix = {}, ImVec2 window_offset = {})
	{
		ImDrawList* draw = ImGui::GetBackgroundDrawList();
		ImDrawListFlags old_flags = draw->Flags;

		ImU32 final_color = color;
		if (fade)
		{
			float t = (sinf((float)ImGui::GetTime() * fade_speed * 5.0f) + 1.0f) * 0.5f;
			ImVec4 c = ImGui::ColorConvertU32ToFloat4(final_color);
			c.w *= t;
			final_color = ImGui::ColorConvertFloat4ToU32(c);
		}

		std::vector<ImVec2> all_projected_points;

		for (auto& pair : *entity.parts)
		{
			const auto& name = pair.first;
			const auto& data = pair.second;
			if (name == "HumanoidRootPart") continue;
			if (data.mesh_id.empty()) continue;

			mesh_utils::mesh_data* mesh = mesh_utils::get_mesh(data.mesh_id);
			if (!mesh || !mesh->loaded || mesh->vertices.empty()) continue;
			float dist = entity.distance;
			if (dist > 5000.0f) continue;

			rbx::vector3_t pos = data.position;
			rbx::vector3_t size = data.size;
			rbx::vector3_t scale = data.scale;
			rbx::matrix3_t rot = data.rotation;
			static thread_local std::vector<ImVec2> projected_vertices;
			projected_vertices.clear();
			projected_vertices.resize(mesh->vertices.size());

			bool any_visible = false;
			for (size_t i = 0; i < mesh->vertices.size(); ++i)
			{
				const auto& v = mesh->vertices[i];
				rbx::vector3_t scaled = { v.pos.x * size.x * scale.x, v.pos.y * size.y * scale.y, v.pos.z * size.z * scale.z };
				rbx::vector3_t rotated = {
					scaled.x * rot.data[0] + scaled.y * rot.data[1] + scaled.z * rot.data[2],
					scaled.x * rot.data[3] + scaled.y * rot.data[4] + scaled.z * rot.data[5],
					scaled.x * rot.data[6] + scaled.y * rot.data[7] + scaled.z * rot.data[8]
				};
				rbx::vector3_t world_pos = { rotated.x + pos.x, rotated.y + pos.y, rotated.z + pos.z };

				rbx::vector2_t screen = globals::visualengine.world_to_screen(world_pos, dimensions, viewmatrix);
				if (screen.x != -1.0f && screen.y != -1.0f)
				{
					projected_vertices[i] = { std::round(screen.x + window_offset.x), std::round(screen.y + window_offset.y) };
					if (glow || globals::visuals::options::box_glow)
						all_projected_points.push_back(projected_vertices[i]);
					any_visible = true;
				}
				else
				{
					projected_vertices[i] = { -10000.0f, -10000.0f };
				}
			}

			if (!any_visible) continue;

			ImDrawListFlags current_flags = draw->Flags;
			draw->Flags &= ~(ImDrawListFlags_AntiAliasedFill | ImDrawListFlags_AntiAliasedLines);

			for (size_t i = 0; i < mesh->indices.size(); i += 3)
			{
				if (i + 2 >= mesh->indices.size()) break;

				uint32_t idx1 = mesh->indices[i];
				uint32_t idx2 = mesh->indices[i + 1];
				uint32_t idx3 = mesh->indices[i + 2];

				if (idx1 >= projected_vertices.size() || idx2 >= projected_vertices.size() || idx3 >= projected_vertices.size())
					continue;

				const ImVec2& p1 = projected_vertices[idx1];
				const ImVec2& p2 = projected_vertices[idx2];
				const ImVec2& p3 = projected_vertices[idx3];

				if (p1.x == -10000.0f || p2.x == -10000.0f || p3.x == -10000.0f) continue;

				draw->AddTriangleFilled(p1, p2, p3, final_color);
			}
			draw->Flags = current_flags;
		}

		if ((glow || globals::visuals::options::box_glow) && all_projected_points.size() >= 3)
		{
			float current_alpha = (float)((final_color >> 24) & 0xFF) / 255.0f;
			ImU32 alpha_val = (ImU32)(globals::visuals::glow_strength * 255.0f * current_alpha);

			if (alpha_val > 0)
			{
				ImU32 final_glow_col = (glow_color != 0 ? glow_color : final_color) & 0x00FFFFFF | (alpha_val << 24);
				std::vector<ImVec2> hull = convex_hull(all_projected_points);
				if (hull.size() >= 3)
				{
					::draw->shadow_convex_poly(draw, hull.data(), (int)hull.size(), final_glow_col, 15.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
					::draw->shadow_convex_poly(draw, hull.data(), (int)hull.size(), final_glow_col, 30.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
				}
			}
		}

		draw->Flags = old_flags;
	}

	static void view_angle_lines(cache::entity_t& entity, ImU32 color, float length, float thickness, rbx::vector2_t dims, rbx::matrix4_t view, ImVec2 window_offset)
	{
		if (entity.instance.address == 0 || entity.parts->find("Head") == entity.parts->end())
			return;

		const auto& head = entity.parts->at("Head");
		rbx::vector3_t start_pos = head.position;
		
		rbx::vector3_t forward = {
			-head.rotation.data[2],
			-head.rotation.data[5],
			-head.rotation.data[8]
		};

		rbx::vector3_t end_pos = start_pos + (forward * length);

		rbx::vector2_t start_screen = globals::visualengine.world_to_screen(start_pos, dims, view);
		rbx::vector2_t end_screen = globals::visualengine.world_to_screen(end_pos, dims, view);

		if (start_screen.x != -1.0f && end_screen.x != -1.0f)
		{
			ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
			draw_list->AddLine(
				ImVec2(start_screen.x + window_offset.x, start_screen.y + window_offset.y),
				ImVec2(end_screen.x + window_offset.x, end_screen.y + window_offset.y),
				color,
				thickness
			);
		}
	}

	static void movement_trails(cache::entity_t& entity, ImU32 color, float thickness, int max_segments, rbx::vector2_t dims, rbx::matrix4_t view, ImVec2 window_offset)
	{
		if (entity.instance.address == 0 || entity.root_part.primitive == 0)
			return;

		static std::unordered_map<std::uintptr_t, std::vector<rbx::vector3_t>> history;
		auto& points = history[entity.instance.address];

		rbx::vector3_t current_pos = entity.root_part.position;

		if (points.empty() || (current_pos - points.back()).length() > 0.5f)
		{
			points.push_back(current_pos);
			if (points.size() > (size_t)max_segments)
				points.erase(points.begin());
		}

		if (points.size() < 2) return;

		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
		for (size_t i = 0; i < points.size() - 1; ++i)
		{
			rbx::vector2_t p1 = globals::visualengine.world_to_screen(points[i], dims, view);
			rbx::vector2_t p2 = globals::visualengine.world_to_screen(points[i + 1], dims, view);

			if (p1.x != -1.0f && p2.x != -1.0f)
			{
				float alpha_scale = (float)i / (float)points.size();
				ImVec4 col = ImGui::ColorConvertU32ToFloat4(color);
				col.w *= alpha_scale;
				
				draw_list->AddLine(
					ImVec2(p1.x + window_offset.x, p1.y + window_offset.y),
					ImVec2(p2.x + window_offset.x, p2.y + window_offset.y),
					ImGui::ColorConvertFloat4ToU32(col),
					thickness
				);
			}
		}
	}

	static void offscreen_arrows(rbx::vector3_t world_pos, ImU32 color, bool glow, ImU32 glow_color, float size, float position, std::string name, float distance, std::vector<int> info, rbx::vector2_t dims, rbx::matrix4_t view, ImVec2 window_offset)
	{
		rbx::vector2_t screen = globals::visualengine.world_to_screen(world_pos, dims, view);
		if (screen.x != -1 && screen.y != -1) return;

		ImVec2 center = ImVec2(dims.x * 0.5f, dims.y * 0.5f);
		rbx::vector3_t camera_pos = globals::camera.get_position();
		rbx::vector3_t delta = (world_pos - camera_pos).normalize();

		float angle = atan2f(delta.z, delta.x);



		float final_radius = position;
		ImVec2 dir = ImVec2(cosf(angle), sinf(angle));

		ImVec2 p1 = ImVec2(center.x + dir.x * final_radius, center.y + dir.y * final_radius);
		ImVec2 p2 = ImVec2(p1.x - (dir.x + dir.y) * size, p1.y - (dir.y - dir.x) * size);
		ImVec2 p3 = ImVec2(p1.x - (dir.x - dir.y) * size, p1.y - (dir.y + dir.x) * size);

		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

		if (glow)
		{
			ImU32 alpha = (ImU32)(globals::visuals::glow_strength * 255.0f);
			if (alpha > 0)
			{
				ImU32 final_glow_col = (glow_color != 0 ? glow_color : color) & 0x00FFFFFF | (alpha << 24);
				ImVec2 pts[3] = { p1, p2, p3 };
				::draw->shadow_convex_poly(draw_list, pts, 3, final_glow_col, 15.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
			}
		}

		draw_list->AddTriangleFilled(p1, p2, p3, color);
		if (should_render_outline(3)) {
			if (globals::visuals::options::global_outline_type == 1) {
				draw_list->AddTriangle(p1, p2, p3, IM_COL32(0, 0, 0, 255), globals::visuals::options::outline_thickness);
			} else if (globals::visuals::options::global_outline_type == 2) {
				draw_list->AddTriangle({ p1.x + 1, p1.y + 1 }, { p2.x + 1, p2.y + 1 }, { p3.x + 1, p3.y + 1 }, IM_COL32(0, 0, 0, 255), globals::visuals::options::outline_thickness);
			}
		}
		if (!info.empty())
		{
			float text_y_offset = 5.0f;
			ImVec2 text_pos = ImVec2(center.x + dir.x * (final_radius + 15.0f), center.y + dir.y * (final_radius + 15.0f));
			ImGuiContext& g = *GImGui;
			ImFont* font = var->font.tahoma;
			float font_size = 12.0f;

			ImGui::PushFont(font);
			if (info[0])
			{
				ImVec2 name_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, name.c_str());
				ImVec2 draw_pos = ImVec2(text_pos.x - name_size.x * 0.5f, text_pos.y);
				draw->text(draw_list, font, font_size, draw_pos, color, name.c_str());
				text_pos.y += name_size.y;
			}

			if (info[1])
			{
				std::string dist_str = std::to_string((int)distance) + "st";
				ImVec2 dist_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, dist_str.c_str());
				ImVec2 draw_pos = ImVec2(text_pos.x - dist_size.x * 0.5f, text_pos.y);
				draw->text(draw_list, font, font_size, draw_pos, color, dist_str.c_str());
			}
			ImGui::PopFont();
		}
	}

	static void sound_esp(cache::entity_t& entity, ImU32 color, bool glow = false, ImU32 glow_color = 0, float radius = 5.0f, float speed = 1.0f, rbx::vector2_t dims = {}, rbx::matrix4_t view = {}, ImVec2 window_offset = {})
	{
		static std::unordered_map<std::uintptr_t, std::vector<std::pair<rbx::vector3_t, float>>> sound_events;
		float current_time = (float)ImGui::GetTime();
		for (auto& pair : sound_events) {
			auto& events = pair.second;
			events.erase(std::remove_if(events.begin(), events.end(), [current_time](const auto& event) {
				return (current_time - event.second) > 2.0f;
				}), events.end());
		}
		static std::unordered_map<std::uintptr_t, rbx::vector3_t> last_positions;
		rbx::vector3_t current_pos = entity.root_part.position;

		if (last_positions.count(entity.instance.address)) {
			float dist = (current_pos - last_positions[entity.instance.address]).length();
			if (dist > 0.5f) {
				static std::unordered_map<std::uintptr_t, float> last_step_time;
				if (current_time - last_step_time[entity.instance.address] > (0.4f / speed)) {
					sound_events[entity.instance.address].push_back({ current_pos, current_time });
					last_step_time[entity.instance.address] = current_time;
				}
			}
		}
		last_positions[entity.instance.address] = current_pos;
		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

		for (const auto& event : sound_events[entity.instance.address]) {
			float life = (current_time - event.second) / 2.0f;
			float current_radius = life * radius * 10.0f;
			ImU32 alpha = (ImU32)((1.0f - life) * 255.0f);
			ImU32 final_color = (color & 0x00FFFFFF) | (alpha << 24);
			const int segments = 32;
			std::vector<ImVec2> points;
			for (int i = 0; i < segments; i++) {
				float a = (i / (float)segments) * IM_PI * 2.0f;
				rbx::vector3_t p_world = { event.first.x + cosf(a) * current_radius, event.first.y - 3.0f, event.first.z + sinf(a) * current_radius };
				rbx::vector2_t p_screen = globals::visualengine.world_to_screen(p_world, dims, view);
				if (p_screen.x != -1.0f) {
					points.push_back({ p_screen.x + window_offset.x, p_screen.y + window_offset.y });
				}
			}

			if (points.size() > 2) {
				draw_list->AddPolyline(points.data(), (int)points.size(), final_color, ImDrawFlags_Closed, 1.5f);
				if (glow) {
					ImU32 g_alpha = (ImU32)((1.0f - life) * globals::visuals::glow_strength * 255.0f);
					ImU32 g_color = (glow_color != 0 ? glow_color : color) & 0x00FFFFFF | (g_alpha << 24);
					if (g_alpha > 0)
						draw->shadow_convex_poly(draw_list, points.data(), (int)points.size(), g_color, 10.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
				}
			}
		}
	}

	static void footprints(cache::entity_t& entity, ImU32 color, bool glow, ImU32 glow_color, float radius, float speed, rbx::vector2_t dims, rbx::matrix4_t view, ImVec2 window_offset)
	{
		struct footprint_event_t {
			rbx::vector3_t position;
			float time;
			bool is_left;
			rbx::vector3_t forward;
		};

		static std::unordered_map<std::uintptr_t, std::vector<footprint_event_t>> footprint_events;
		static std::unordered_map<std::uintptr_t, rbx::vector3_t> last_positions;
		static std::unordered_map<std::uintptr_t, bool> last_side;
		static std::unordered_map<std::uintptr_t, float> last_step_time;

		rbx::vector3_t current_pos = entity.root_part.position;
		float current_time = (float)ImGui::GetTime();
		if (last_positions.count(entity.instance.address)) {
			float dist = (current_pos - last_positions[entity.instance.address]).length();
			if (dist > 1.5f) {
				if (current_time - last_step_time[entity.instance.address] > (0.3f / speed)) {
					bool side = !last_side[entity.instance.address];
					last_side[entity.instance.address] = side;

					rbx::vector3_t forward = (current_pos - last_positions[entity.instance.address]).normalize();
					if (forward.magnitude() < 0.1f) forward = { 0, 0, 1 };

					footprint_events[entity.instance.address].push_back({ current_pos, current_time, side, forward });
					last_step_time[entity.instance.address] = current_time;
				}
			}
		}
		last_positions[entity.instance.address] = current_pos;
		auto& events = footprint_events[entity.instance.address];
		events.erase(std::remove_if(events.begin(), events.end(), [current_time](const footprint_event_t& e) {
			return (current_time - e.time) > 4.0f;
			}), events.end());

		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

		for (const auto& event : events) {
			float life = (current_time - event.time) / 4.0f;
			ImU32 alpha = (ImU32)((1.0f - life) * 255.0f);
			ImU32 final_color = (color & 0x00FFFFFF) | (alpha << 24);
			rbx::vector3_t right = event.forward.cross({ 0, 1, 0 }).normalize();
			float offset_dist = 0.6f;
			rbx::vector3_t foot_pos = event.position + (event.is_left ? right * -offset_dist : right * offset_dist);
			foot_pos.y -= 3.0f;
			const int segments = 8;
			std::vector<ImVec2> points;
			float f_radius_x = 0.3f * radius;
			float f_radius_z = 0.5f * radius;

			for (int i = 0; i < segments; i++) {
				float a = (i / (float)segments) * IM_PI * 2.0f;
				rbx::vector3_t p_local = { cosf(a) * f_radius_x, 0, sinf(a) * f_radius_z };
				rbx::vector3_t p_rot;
				p_rot.x = p_local.x * right.x + p_local.z * event.forward.x;
				p_rot.y = 0;
				p_rot.z = p_local.x * right.z + p_local.z * event.forward.z;

				rbx::vector2_t p_screen = globals::visualengine.world_to_screen(foot_pos + p_rot, dims, view);
				if (p_screen.x != -1.0f) {
					points.push_back({ p_screen.x + window_offset.x, p_screen.y + window_offset.y });
				}
			}

			if (points.size() > 2) {
				draw_list->AddConvexPolyFilled(points.data(), (int)points.size(), final_color);
				if (glow) {
					ImU32 g_alpha = (ImU32)((1.0f - life) * globals::visuals::glow_strength * 255.0f);
					ImU32 g_color = (glow_color != 0 ? glow_color : color) & 0x00FFFFFF | (g_alpha << 24);
					if (g_alpha > 0)
						draw->shadow_convex_poly(draw_list, points.data(), (int)points.size(), g_color, 8.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
				}
			}
		}
	}

	static void bounding_box(ImVec2& c1, ImVec2& c2, ImU32 color, bool glow = false, ImU32 glow_color = 0, bool fill = false, bool fill_gradient = false, bool fill_gradient_spin = false, float gradient_speed = 2.0f, int gradient_style = 0, ImU32* fill_colors = nullptr)
	{
		c1.x = std::round(c1.x); c1.y = std::round(c1.y);
		c2.x = std::round(c2.x); c2.y = std::round(c2.y);

		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
		draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;

		ImRect rect(c1.x, c1.y, c1.x + c2.x, c1.y + c2.y);

		if (fill || globals::visuals::options::bounding_box_fill)
		{
			if (fill_gradient || globals::visuals::options::box_fill_gradient)
			{
				float time = (fill_gradient_spin || globals::visuals::options::box_fill_gradient_spin) ? (float)ImGui::GetTime() * gradient_speed : 0.0f;
				ImU32 col1 = fill_colors ? fill_colors[0] : globals::visuals::colors::box_fill[0];
				ImU32 col2 = fill_colors ? fill_colors[1] : globals::visuals::colors::box_fill[1];

				float s = sinf(time);
				float c = cosf(time);

				ImU32 c_tl, c_tr, c_br, c_bl;

				if (gradient_style == 0)
				{
					c_tl = c_bl = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (s + 1.0f) * 0.5f));
					c_tr = c_br = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (c + 1.0f) * 0.5f));
				}
				else if (gradient_style == 1)
				{
					c_tl = c_tr = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (s + 1.0f) * 0.5f));
					c_bl = c_br = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (c + 1.0f) * 0.5f));
				}
				else
				{
					c_tl = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (s + 1.0f) * 0.5f));
					c_tr = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (c + 1.0f) * 0.5f));
					c_br = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (-s + 1.0f) * 0.5f));
					c_bl = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (-c + 1.0f) * 0.5f));
				}

				draw_list->AddRectFilledMultiColor(rect.Min, rect.Max, c_tl, c_tr, c_br, c_bl);
			}
			else
				draw_list->AddRectFilled(rect.Min, rect.Max, fill_colors ? fill_colors[0] : globals::visuals::colors::box_fill[0], 0.f);
		}

		if (should_render_outline(1)) {
			if (globals::visuals::options::global_outline_type == 1) {
				draw_list->AddRect({ rect.Min.x - 2.f, rect.Min.y - 2.f }, { rect.Max.x + 2.f, rect.Max.y + 2.f }, IM_COL32(0, 0, 0, 255));
				draw_list->AddRect({ rect.Min.x, rect.Min.y }, { rect.Max.x, rect.Max.y }, IM_COL32(0, 0, 0, 255));
			} else if (globals::visuals::options::global_outline_type == 2) {
				draw_list->AddRect({ rect.Min.x + 1.f, rect.Min.y + 1.f }, { rect.Max.x + 1.f, rect.Max.y + 1.f }, IM_COL32(0, 0, 0, 255));
			}
		}

		if (glow || globals::visuals::options::box_glow)
		{
			ImU32 alpha = (ImU32)(globals::visuals::glow_strength * 255.0f);
			if (alpha > 0)
			{
				ImU32 final_glow_col = (glow_color != 0 ? glow_color : color) & 0x00FFFFFF | (alpha << 24);
				draw->shadow_rect(draw_list, rect.Min, rect.Max, final_glow_col, 15.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
			}
		}

		draw_list->AddRect({ rect.Min.x - 1.f, rect.Min.y - 1.f }, { rect.Max.x + 1.f, rect.Max.y + 1.f }, color);

		if (globals::visuals::options::box_inline)
		{
			draw_list->AddRect({ rect.Min.x, rect.Min.y }, { rect.Max.x, rect.Max.y }, IM_COL32(0, 0, 0, 255));
		}
	}

	static void corner_box(ImVec2 c1, ImVec2 c2, ImU32 col, float length_factor = 0.3f, bool glow = false, ImU32 glow_color = 0, bool fill = false, bool fill_gradient = false, bool fill_gradient_spin = false, float gradient_speed = 2.0f, int gradient_style = 0, ImU32* fill_colors = nullptr)
	{
		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
		draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;

		float x1 = std::round(c1.x - 1.f);
		float y1 = std::round(c1.y - 1.f);
		float x2 = std::round(c1.x + c2.x + 1.f);
		float y2 = std::round(c1.y + c2.y + 1.f);

		ImU32 outline_col = IM_COL32(0, 0, 0, 255);

		float box_width = x2 - x1;
		float box_height = y2 - y1;
		float length = std::min(box_width * length_factor, box_height * length_factor);
		length = std::max(length, 5.f);
		length = std::min(length, 500.f);

		if (fill || globals::visuals::options::bounding_box_fill)
		{
			ImVec2 fill_min(x1 + 2.f, y1 + 2.f);
			ImVec2 fill_max(x2 - 2.f, y2 - 2.f);
			if (fill_gradient || globals::visuals::options::box_fill_gradient)
			{
				float time = (fill_gradient_spin || globals::visuals::options::box_fill_gradient_spin) ? (float)ImGui::GetTime() * gradient_speed : 0.0f;
				ImU32 col1 = fill_colors ? fill_colors[0] : globals::visuals::colors::box_fill[0];
				ImU32 col2 = fill_colors ? fill_colors[1] : globals::visuals::colors::box_fill[1];

				float s = sinf(time);
				float c = cosf(time);

				ImU32 c_tl, c_tr, c_br, c_bl;

				if (gradient_style == 0)
				{
					c_tl = c_bl = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (s + 1.0f) * 0.5f));
					c_tr = c_br = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (c + 1.0f) * 0.5f));
				}
				else if (gradient_style == 1)
				{
					c_tl = c_tr = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (s + 1.0f) * 0.5f));
					c_bl = c_br = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (c + 1.0f) * 0.5f));
				}
				else
				{
					c_tl = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (s + 1.0f) * 0.5f));
					c_tr = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (c + 1.0f) * 0.5f));
					c_br = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (-s + 1.0f) * 0.5f));
					c_bl = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (-c + 1.0f) * 0.5f));
				}

				draw_list->AddRectFilledMultiColor(fill_min, fill_max, c_tl, c_tr, c_br, c_bl);
			}
			else
				draw_list->AddRectFilled(fill_min, fill_max, fill_colors ? fill_colors[0] : globals::visuals::colors::box_fill[0]);
		}

		auto draw_corner = [&](float x, float y, float w, float h, ImU32 color, float thickness) {
			draw_list->AddLine({ x, y }, { x + w, y }, color, thickness);
			draw_list->AddLine({ x, y }, { x, y + h }, color, thickness);
		};

		if (should_render_outline(1)) {
			if (globals::visuals::options::global_outline_type == 1) {
				draw_corner(x1 - 1, y1 - 1, length + 1, length + 1, outline_col, globals::visuals::options::outline_thickness);
				draw_corner(x1 + 1, y1 + 1, length - 1, length - 1, outline_col, globals::visuals::options::outline_thickness);
				draw_corner(x2 + 1, y1 - 1, -length - 1, length + 1, outline_col, globals::visuals::options::outline_thickness);
				draw_corner(x2 - 1, y1 + 1, -length + 1, length - 1, outline_col, globals::visuals::options::outline_thickness);
				draw_corner(x1 - 1, y2 + 1, length + 1, -length - 1, outline_col, globals::visuals::options::outline_thickness);
				draw_corner(x1 + 1, y2 - 1, length - 1, -length + 1, outline_col, globals::visuals::options::outline_thickness);
				draw_corner(x2 + 1, y2 + 1, -length - 1, -length - 1, outline_col, globals::visuals::options::outline_thickness);
				draw_corner(x2 - 1, y2 - 1, -length + 1, -length + 1, outline_col, globals::visuals::options::outline_thickness);
			} else if (globals::visuals::options::global_outline_type == 2) {
				draw_corner(x1 + 1, y1 + 1, length, length, outline_col, globals::visuals::options::outline_thickness);
				draw_corner(x2 + 1, y1 + 1, -length, length, outline_col, globals::visuals::options::outline_thickness);
				draw_corner(x1 + 1, y2 + 1, length, -length, outline_col, globals::visuals::options::outline_thickness);
				draw_corner(x2 + 1, y2 + 1, -length, -length, outline_col, globals::visuals::options::outline_thickness);
			}
		}

		if (glow || globals::visuals::options::box_glow)
		{
			ImU32 alpha = (ImU32)(globals::visuals::glow_strength * 255.0f);
			ImU32 final_glow_col = (glow_color != 0 ? glow_color : col) & 0x00FFFFFF | (alpha << 24);
			draw->shadow_rect(draw_list, ImVec2(x1, y1), ImVec2(x2, y2), final_glow_col, 15.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
		}

		draw_corner(x1, y1, length, length, col, globals::visuals::options::outline_thickness);
		draw_corner(x2, y1, -length, length, col, globals::visuals::options::outline_thickness);
		draw_corner(x1, y2, length, -length, col, globals::visuals::options::outline_thickness);
		draw_corner(x2, y2, -length, -length, col, globals::visuals::options::outline_thickness);
	}

	inline ImFont* get_current_font() {
		return (globals::visuals::options::font_type == 1) ? var->font.visitor : var->font.tahoma;
	}

	inline float get_current_font_height() {
		ImFont* font = get_current_font();
		return font->FontSize;
	}

	static void outlined_text(const ImVec2& pos, const ImVec2& size, const char* text, ImU32 color, float y_offset = 0.f, bool use_outline = true, ImU32 outline_col = IM_COL32(0, 0, 0, 255), bool glow = false, int type = -1)
	{
		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
		draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;

		ImFont* font = get_current_font();

		ImGui::PushFont(font);
		ImVec2 text_size = ImGui::CalcTextSize(text);

		float ref_width = size.x > 0.f ? size.x : text_size.x;

		float x = pos.x;

		ImVec2 final_pos(std::round(x), std::round(pos.y + y_offset));

		if (glow)
		{
			ImU32 alpha = (ImU32)(globals::visuals::glow_strength * 255.0f);
			if (alpha > 0)
			{
				ImU32 glow_col = color & 0x00FFFFFF | (alpha << 24);
				draw->shadow_rect(draw_list, final_pos, ImVec2(final_pos.x + text_size.x, final_pos.y + text_size.y), glow_col, 15.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
			}
		}

		bool render_outline = use_outline;
		if (type != -1) render_outline = render_outline && should_render_outline(type);

		if (render_outline && globals::visuals::options::global_outline_type != 0) {
			if (globals::visuals::options::global_outline_type == 1) {
				const ImVec2 offsets[8] = {
					ImVec2(-1,0), ImVec2(1,0), ImVec2(0,-1), ImVec2(0,1),
					ImVec2(-1,-1), ImVec2(1,-1), ImVec2(-1,1), ImVec2(1,1)
				};
				for (auto& o : offsets) {
					ImVec2 outline_pos(std::round(final_pos.x + o.x * globals::visuals::options::text_outline_thickness), std::round(final_pos.y + o.y * globals::visuals::options::text_outline_thickness));
					draw_list->AddText(font, font->FontSize, outline_pos, outline_col, text);
				}
			} else if (globals::visuals::options::global_outline_type == 2) {
				ImVec2 shadow_pos(std::round(final_pos.x + globals::visuals::options::text_outline_thickness), std::round(final_pos.y + globals::visuals::options::text_outline_thickness));
				draw_list->AddText(font, font->FontSize, shadow_pos, outline_col, text);
			}
		}

		draw_list->AddText(font, font->FontSize, final_pos, color, text);
		ImGui::PopFont();
	}
	static void healthbar(std::uintptr_t address, const ImVec2& box_pos, const ImVec2& box_size, float health, float max_health, ImU32 health_color, float gap = 1.0f, float thickness = 3.0f, ImU32 outline_col = IM_COL32(0, 0, 0, 255), bool glow = false, ImU32 glow_color = 0, bool gradient = false, ImU32* gradient_colors = nullptr, bool text = false, ImU32 text_color = IM_COL32(255, 255, 255, 255), bool lerp = false, bool is_corner = false)
	{
		if (globals::visuals::options::ignore_full && health >= max_health)
			return;

		auto draw_list = ImGui::GetBackgroundDrawList();
		draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;

		static std::unordered_map<std::uintptr_t, float> lerped_health;

		float target_health = health;
		if (lerp)
		{
			if (lerped_health.find(address) == lerped_health.end())
				lerped_health[address] = health;

			lerped_health[address] = ImLerp(lerped_health[address], target_health, ImGui::GetIO().DeltaTime * 10.0f);
			health = lerped_health[address];
		}

		float ratio = (max_health > 0.f) ? health / max_health : 0.f;
		ratio = std::fmax(0.f, std::fmin(ratio, 1.f));

		float y_min = std::round(box_pos.y);
		float y_max = std::round(box_pos.y + box_size.y);
		float x_min = std::round(box_pos.x);

		float x_hp = x_min - gap - thickness - 3.0f;

		if (should_render_outline(2)) {
			ImVec2 bg_min(x_hp - 1.f, y_min - 2.f);
			ImVec2 bg_max(x_hp + thickness + 1.f, y_max + (is_corner ? 2.f : 1.f) + 1.f);
			draw_list->AddRectFilled(bg_min, bg_max, outline_col);
		}

		ImU32 empty_col = IM_COL32(130, 130, 130, 255);
		ImVec2 empty_min(x_hp, y_min - 1.f);
		ImVec2 empty_max(x_hp + thickness, y_max + (is_corner ? 2.f : 1.f));
		draw_list->AddRectFilled(empty_min, empty_max, empty_col);

		float height = (y_max - y_min) * ratio;
		ImVec2 fg_min(x_hp, y_max - height - (is_corner ? 0.f : 1.f));
		ImVec2 fg_max(x_hp + thickness, y_max + (is_corner ? 2.f : 1.f));

		if (gradient || globals::visuals::options::healthbar_gradient || globals::visuals::options::gradient_bars)
		{
			float mid_y = fg_min.y + (fg_max.y - fg_min.y) * 0.5f;
			if (glow || globals::visuals::options::healthbar_glow)
			{
				ImU32 alpha = (ImU32)(globals::visuals::glow_strength * 255.0f);
				if (alpha > 0)
				{
					ImU32 final_glow_col = (glow_color != 0 ? glow_color : (gradient_colors ? gradient_colors[1] : globals::visuals::colors::healthbar[1])) & 0x00FFFFFF | (alpha << 24);
					draw->shadow_rect(draw_list, fg_min, fg_max, final_glow_col, 10.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
				}
			}

			ImU32 g_col0 = gradient_colors ? gradient_colors[0] : IM_COL32(255, 0, 0, 255);
			ImU32 g_col1 = gradient_colors ? gradient_colors[1] : IM_COL32(255, 255, 0, 255);
			ImU32 g_col2 = gradient_colors ? gradient_colors[2] : IM_COL32(0, 255, 0, 255);

			draw_list->AddRectFilledMultiColor(fg_min, { fg_max.x, mid_y }, g_col2, g_col2, g_col1, g_col1);
			draw_list->AddRectFilledMultiColor({ fg_min.x, mid_y }, fg_max, g_col1, g_col1, g_col0, g_col0);
		}
		else
		{
			if (glow || globals::visuals::options::healthbar_glow)
			{
				ImU32 alpha = (ImU32)(globals::visuals::glow_strength * 255.0f);
				if (alpha > 0)
				{
					ImU32 final_glow_col = (glow_color != 0 ? glow_color : health_color) & 0x00FFFFFF | (alpha << 24);
					draw->shadow_rect(draw_list, fg_min, fg_max, final_glow_col, 10.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
				}
			}
			draw_list->AddRectFilled(fg_min, fg_max, health_color);
		}

		if (text || globals::visuals::options::healthbar_text)
		{
			std::string health_str;
			if (globals::visuals::options::value_percentages)
				health_str = std::to_string((int)(ratio * 100.f)) + "%";
			else
				health_str = std::to_string((int)health);

			ImGui::PushFont(get_current_font());
			ImVec2 text_size = ImGui::CalcTextSize(health_str.c_str());
			ImGui::PopFont();

			ImVec2 text_pos;
			float ref_y = globals::visuals::options::value_follow ? fg_min.y : y_min;
			float y_offset = (globals::visuals::options::font_type == 0) ? 2.0f : 1.0f;
			text_pos = ImVec2(x_hp - text_size.x - 4.0f, ref_y - (text_size.y * 0.5f) + y_offset);

			outlined_text(text_pos, { 0,0 }, health_str.c_str(), text_color != IM_COL32(255,255,255,255) ? text_color : globals::visuals::colors::healthbar_text, 0.f, true, IM_COL32(0, 0, 0, 255), glow || globals::visuals::options::healthbar_glow, 2);
		}
	}

	static void head_visuals(cache::entity_t& entity, ImU32 color, bool glow = false, ImU32 glow_color = 0, rbx::vector2_t dims = {}, rbx::matrix4_t view = {}, ImVec2 window_offset = {})
	{
		if (entity.parts->count("Head") == 0) return;
		const auto& head = (*entity.parts)["Head"];

		auto draw_list = ImGui::GetBackgroundDrawList();

		if (globals::visuals::options::head_type == 0) // Dot
		{
			rbx::vector2_t screen = globals::visualengine.world_to_screen(head.position, dims, view);
			if (screen.x != -1 && screen.y != -1)
			{
				screen.x += window_offset.x;
				screen.y += window_offset.y;

				ImVec2 center = { std::round(screen.x), std::round(screen.y) };
				float radius = 3.0f;

				if (glow)
				{
					ImU32 alpha = (ImU32)(globals::visuals::glow_strength * 255.0f);
					if (alpha > 0)
					{
						ImU32 final_glow_col = (glow_color != 0 ? glow_color : color) & 0x00FFFFFF | (alpha << 24);
						draw->shadow_circle(draw_list, center, radius, final_glow_col, 15.0f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground);
					}
				}

				if (should_render_outline(4)) {
					if (globals::visuals::options::global_outline_type == 1) {
						if (globals::visuals::options::circular_head_dot)
							draw_list->AddCircle(center, radius + 1.0f, IM_COL32(0, 0, 0, 255), 12, globals::visuals::options::outline_thickness);
						else
							draw_list->AddRect(ImVec2(center.x - radius - 1.0f, center.y - radius - 1.0f), ImVec2(center.x + radius + 1.0f, center.y + radius + 1.0f), IM_COL32(0, 0, 0, 255));
					}
					else if (globals::visuals::options::global_outline_type == 2) {
						if (globals::visuals::options::circular_head_dot)
							draw_list->AddCircle({ center.x + 1, center.y + 1 }, radius, IM_COL32(0, 0, 0, 255), 12, globals::visuals::options::outline_thickness);
						else
							draw_list->AddRect(ImVec2(center.x - radius + 1.0f, center.y - radius + 1.0f), ImVec2(center.x + radius + 1.0f, center.y + radius + 1.0f), IM_COL32(0, 0, 0, 255));
					}
				}

				if (globals::visuals::options::circular_head_dot)
					draw_list->AddCircleFilled(center, radius, color, 12);
				else
					draw_list->AddRectFilled(ImVec2(center.x - radius, center.y - radius), ImVec2(center.x + radius, center.y + radius), color);
			}
		}
		else if (globals::visuals::options::head_type == 1) // Hexagon
		{
			rbx::vector3_t head_pos = head.position;
			rbx::matrix3_t head_rot = globals::get_camera().get_rotation();
			float radius = (globals::visuals::options::head_mode == 1) ? (std::min(std::max({ head.size.x, head.size.y, head.size.z }) * 1.0f, globals::visuals::options::head_size_limit)) : globals::visuals::options::head_static_size;

			ImVec2 screen_points[6];
			bool all_valid = true;

			// Use a vertical offset that matches the hexagon's bottom flat edge
			float vertical_offset = radius * sinf(IM_PI / 3.0f); // Height from center to flat edge

			for (int i = 0; i < 6; i++)
			{
				float angle = i * (IM_PI * 2.0f / 6.0f);
				rbx::vector3_t local_pos = { cosf(angle) * radius, sinf(angle) * radius, 0.0f };
				rbx::vector3_t world = head_pos + head_rot * local_pos;

				rbx::vector2_t out = globals::visualengine.world_to_screen(world, dims, view);
				if (out.x == -1.0f) { all_valid = false; break; }

				screen_points[i] = ImVec2(out.x + window_offset.x, out.y + window_offset.y);
			}

			if (all_valid)
			{
				ImU32 outline_col = IM_COL32(0, 0, 0, 255);
				float thickness = globals::visuals::options::outline_thickness;

				auto draw_edge = [&](int i, int j) {
					if (glow)
					{
						ImU32 alpha = (ImU32)(globals::visuals::glow_strength * 255.0f);
						if (alpha > 0)
						{
							ImU32 final_glow_col = (glow_color != 0 ? glow_color : color) & 0x00FFFFFF | (alpha << 24);
							draw->shadow_line(draw_list, screen_points[i], screen_points[j], final_glow_col, 15.0f);
						}
					}

					if (should_render_outline(4))
					{
						draw_list->AddLine(screen_points[i], screen_points[j], outline_col, thickness + 1.5f);
					}
					draw_list->AddLine(screen_points[i], screen_points[j], color, thickness);
				};

				for (int i = 0; i < 6; i++)
				{
					draw_edge(i, (i + 1) % 6);
				}
			}
		}
	}

	static void box_fill(ImVec2& pos, ImVec2& size, ImU32 col)
	{
		pos.x = std::round(pos.x); pos.y = std::round(pos.y);
		size.x = std::round(size.x); size.y = std::round(size.y);
		auto draw = ImGui::GetBackgroundDrawList();
		draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;
		ImRect rect(pos.x, pos.y, pos.x + size.x, pos.y + size.y);

		if (globals::visuals::options::box_fill_gradient)
		{
			float time = globals::visuals::options::box_fill_gradient_spin ? ImGui::GetTime() * 2.0f : 0.0f;
			ImU32 col1 = globals::visuals::colors::box_fill[0];
			ImU32 col2 = globals::visuals::colors::box_fill[1];

			float s = sinf(time);
			float c = cosf(time);

			ImU32 c_tl = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (s + 1.0f) * 0.5f));
			ImU32 c_tr = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (c + 1.0f) * 0.5f));
			ImU32 c_br = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (-s + 1.0f) * 0.5f));
			ImU32 c_bl = ImGui::ColorConvertFloat4ToU32(ImLerp(ImGui::ColorConvertU32ToFloat4(col1), ImGui::ColorConvertU32ToFloat4(col2), (-c + 1.0f) * 0.5f));

			draw->AddRectFilledMultiColor(rect.Min, rect.Max, c_tl, c_tr, c_br, c_bl);
		}
		else
			draw->AddRectFilled(rect.Min, rect.Max, col, 0.f);
	}



	static void name(const ImVec2& box_pos, const ImVec2& box_size, const std::string& name, ImU32 color, bool use_outline = true, bool glow = false)
	{
		ImGui::PushFont(get_current_font());
		ImVec2 text_size = ImGui::CalcTextSize(name.c_str());
		ImGui::PopFont();

		ImVec2 text_pos;
		text_pos.x = box_pos.x + (box_size.x * 0.5f) - (text_size.x * 0.5f);

		text_pos.y = box_pos.y - text_size.y - 3.0f;

		outlined_text(text_pos, text_size, name.c_str(), color, 0.f, use_outline, IM_COL32(0, 0, 0, 255), glow || globals::visuals::options::name_glow, 0);
	}

	static void distance(const ImVec2& box_pos, const ImVec2& box_size, float meters, ImU32 color, bool use_outline = true, bool glow = false)
	{
		char buf[32];
		snprintf(buf, sizeof(buf), "%.0f", meters);

		ImGui::PushFont(get_current_font());
		ImVec2 text_size = ImGui::CalcTextSize(buf);
		ImGui::PopFont();

		ImVec2 text_pos;
		text_pos.x = box_pos.x + (box_size.x * 0.5f) - (text_size.x * 0.5f);

		text_pos.y = box_pos.y + box_size.y + 3.0f;

		outlined_text(text_pos, text_size, buf, color, 0.f, use_outline, IM_COL32(0, 0, 0, 255), glow || globals::visuals::options::distance_glow, 5);
	}

	static void tool(const ImVec2& box_pos, const ImVec2& box_size, std::string tool_name, ImU32 color, bool has_distance, bool use_outline = true)
	{
		if (tool_name.empty() || tool_name == "None")
			tool_name = "Empty";

		ImGui::PushFont(get_current_font());
		ImVec2 text_size = ImGui::CalcTextSize(tool_name.c_str());
		ImGui::PopFont();

		ImVec2 text_pos;
		text_pos.x = box_pos.x + (box_size.x * 0.5f) - (text_size.x * 0.5f);

		float font_height = get_current_font_height();

		float offset = has_distance ? (font_height + 6.0f) : 3.0f;
		text_pos.y = box_pos.y + box_size.y + offset;

		outlined_text(text_pos, text_size, tool_name.c_str(), color, 0.f, use_outline, IM_COL32(0, 0, 0, 255), false, -1);
	}

	static void esp_flags(const ImVec2& box_pos, const ImVec2& box_size, const std::string& state, const std::string& rig_type, ImU32 color, bool is_corner = false)
	{
		ImGui::PushFont(get_current_font());

		auto draw_flag = [&](const std::string& text, int& row) {
			if (text.empty()) return;
			std::string formatted = "[" + text + "]";
			ImVec2 text_size = ImGui::CalcTextSize(formatted.c_str());
			ImVec2 pos = { box_pos.x + box_size.x + (is_corner ? 5.0f : 4.0f), box_pos.y - 3.0f + (row * (text_size.y + 1.0f)) };
			outlined_text(pos, text_size, formatted.c_str(), color, 0.f, true, IM_COL32(0, 0, 0, 255));
			row++;
		};

		int row = 0;
		draw_flag(state, row);
		draw_flag(rig_type, row);

		ImGui::PopFont();
	}

	static void skeleton(cache::entity_t& entity, ImU32 color, bool glow = false, ImU32 glow_color = 0, rbx::vector2_t dims = {}, rbx::matrix4_t view = {}, ImVec2 window_offset = {})
	{
		auto draw_list = ImGui::GetBackgroundDrawList();

		auto draw_polyline_w = [&](const std::vector<rbx::vector3_t>& points, bool closed = false) {
			std::vector<ImVec2> screen_points;
			for (const auto& w : points)
			{
				rbx::vector2_t scr = globals::visualengine.world_to_screen(w, dims, view);
				if (scr.x != -1 && scr.y != -1)
				{
					screen_points.push_back({ std::round(scr.x + window_offset.x), std::round(scr.y + window_offset.y) });
				}
				else
				{
					// If a point is behind the camera, we render what we have and start a new polyline
					if (screen_points.size() >= 2)
					{
						if (glow)
						{
							ImU32 final_glow_col = (glow_color != 0) ? glow_color : color;
							for (size_t i = 0; i < screen_points.size() - 1; ++i)
								draw->shadow_line(draw_list, screen_points[i], screen_points[i + 1], final_glow_col, 10.0f);
						}

						if (should_render_outline(6)) {
							if (globals::visuals::options::global_outline_type == 1) {
								draw_list->AddPolyline(screen_points.data(), (int)screen_points.size(), IM_COL32(0, 0, 0, 255), 0, globals::visuals::options::skeleton_outline_thickness + 2.0f);
							}
							else if (globals::visuals::options::global_outline_type == 2) {
								std::vector<ImVec2> shadow_points = screen_points;
								for (auto& p : shadow_points) { p.x += 1.0f; p.y += 1.0f; }
								draw_list->AddPolyline(shadow_points.data(), (int)shadow_points.size(), IM_COL32(0, 0, 0, 255), 0, globals::visuals::options::skeleton_outline_thickness);
							}
						}

						draw_list->AddPolyline(screen_points.data(), (int)screen_points.size(), color, 0, globals::visuals::options::skeleton_outline_thickness);
					}
					screen_points.clear();
				}
			}

			if (screen_points.size() < 2) return;

			if (glow)
			{
				ImU32 final_glow_col = (glow_color != 0) ? glow_color : color;
				for (size_t i = 0; i < screen_points.size() - 1; ++i)
					draw->shadow_line(draw_list, screen_points[i], screen_points[i + 1], final_glow_col, 10.0f);
				if (closed && screen_points.size() > 2)
					draw->shadow_line(draw_list, screen_points.back(), screen_points.front(), final_glow_col, 10.0f);
			}

			if (should_render_outline(6)) {
				if (globals::visuals::options::global_outline_type == 1) {
					draw_list->AddPolyline(screen_points.data(), (int)screen_points.size(), IM_COL32(0, 0, 0, 255), closed ? ImDrawFlags_Closed : 0, globals::visuals::options::skeleton_outline_thickness + 2.0f);
				}
				else if (globals::visuals::options::global_outline_type == 2) {
					std::vector<ImVec2> shadow_points = screen_points;
					for (auto& p : shadow_points) { p.x += 1.0f; p.y += 1.0f; }
					draw_list->AddPolyline(shadow_points.data(), (int)shadow_points.size(), IM_COL32(0, 0, 0, 255), closed ? ImDrawFlags_Closed : 0, globals::visuals::options::skeleton_outline_thickness);
				}
			}

			draw_list->AddPolyline(screen_points.data(), (int)screen_points.size(), color, closed ? ImDrawFlags_Closed : 0, globals::visuals::options::skeleton_outline_thickness);
		};

		if (entity.rig_type == 0)
		{
			if (entity.parts->count("Torso") && entity.parts->count("Head"))
			{
				auto& torso = (*entity.parts)["Torso"];
				auto& head = (*entity.parts)["Head"];

				rbx::vector3_t shoulder_center = torso.position + torso.rotation * rbx::vector3_t{ 0, torso.size.y * 0.2f, 0 };
				rbx::vector3_t hip_center = torso.position - torso.rotation * rbx::vector3_t{ 0, torso.size.y * 0.4f, 0 };

				rbx::vector3_t shoulder_left = shoulder_center + torso.rotation * rbx::vector3_t{ -torso.size.x * 0.5f, 0, 0 };
				rbx::vector3_t shoulder_right = shoulder_center + torso.rotation * rbx::vector3_t{ torso.size.x * 0.5f, 0, 0 };

				rbx::vector3_t head_bottom = head.position - rbx::vector3_t{ 0, head.size.y * 0.5f, 0 };
				
				if (globals::visuals::options::head_type == 1)
				{
					float radius = (globals::visuals::options::head_mode == 1) ? (std::min(std::max({ head.size.x, head.size.y, head.size.z }) * 1.0f, globals::visuals::options::head_size_limit)) : globals::visuals::options::head_static_size;
					float vertical_offset = radius * sinf(IM_PI / 3.0f);
					rbx::vector3_t hex_bottom = head.position - globals::get_camera().get_rotation() * rbx::vector3_t{ 0, vertical_offset, 0 };
					draw_polyline_w({ hex_bottom, shoulder_center, hip_center });
				}
				else
				{
					draw_polyline_w({ head.position, head_bottom, shoulder_center, hip_center });
				}

				// Left Arm
				std::vector<rbx::vector3_t> left_arm = { shoulder_center, shoulder_left };
				if (entity.parts->count("Left Arm")) {
					auto& arm = (*entity.parts)["Left Arm"];
					left_arm.push_back(arm.position + arm.rotation * rbx::vector3_t{ 0, arm.size.y * 0.2f, 0 });
					left_arm.push_back(arm.position - arm.rotation * rbx::vector3_t{ 0, arm.size.y * 0.5f, 0 });
				}
				draw_polyline_w(left_arm);

				// Right Arm
				std::vector<rbx::vector3_t> right_arm = { shoulder_center, shoulder_right };
				if (entity.parts->count("Right Arm")) {
					auto& arm = (*entity.parts)["Right Arm"];
					right_arm.push_back(arm.position + arm.rotation * rbx::vector3_t{ 0, arm.size.y * 0.2f, 0 });
					right_arm.push_back(arm.position - arm.rotation * rbx::vector3_t{ 0, arm.size.y * 0.5f, 0 });
				}
				draw_polyline_w(right_arm);

				// Left Leg
				std::vector<rbx::vector3_t> left_leg = { hip_center };
				if (entity.parts->count("Left Leg")) {
					auto& leg = (*entity.parts)["Left Leg"];
					left_leg.push_back(leg.position + leg.rotation * rbx::vector3_t{ 0, leg.size.y * 0.5f, 0 });
					left_leg.push_back(leg.position - leg.rotation * rbx::vector3_t{ 0, leg.size.y * 0.5f, 0 });
				}
				draw_polyline_w(left_leg);

				// Right Leg
				std::vector<rbx::vector3_t> right_leg = { hip_center };
				if (entity.parts->count("Right Leg")) {
					auto& leg = (*entity.parts)["Right Leg"];
					right_leg.push_back(leg.position + leg.rotation * rbx::vector3_t{ 0, leg.size.y * 0.5f, 0 });
					right_leg.push_back(leg.position - leg.rotation * rbx::vector3_t{ 0, leg.size.y * 0.5f, 0 });
				}
				draw_polyline_w(right_leg);
			}
		}
		else if (entity.rig_type == 1)
		{
			if (entity.parts->count("UpperTorso") && entity.parts->count("LowerTorso") && entity.parts->count("Head"))
			{
				auto& ut = (*entity.parts)["UpperTorso"];
				auto& lt = (*entity.parts)["LowerTorso"];
				auto& head = (*entity.parts)["Head"];

				rbx::vector3_t shoulder_center = ut.position + ut.rotation * rbx::vector3_t{ 0, ut.size.y * 0.05f, 0 };
				rbx::vector3_t hip_center = lt.position - lt.rotation * rbx::vector3_t{ 0, lt.size.y * 0.25f, 0 };

				rbx::vector3_t shoulder_left = shoulder_center + ut.rotation * rbx::vector3_t{ -ut.size.x * 0.5f, 0, 0 };
				rbx::vector3_t shoulder_right = shoulder_center + ut.rotation * rbx::vector3_t{ ut.size.x * 0.5f, 0, 0 };

				rbx::vector3_t head_bottom = head.position - rbx::vector3_t{ 0, head.size.y * 0.5f, 0 };

				if (globals::visuals::options::head_type == 1)
  				{
  					float radius = (globals::visuals::options::head_mode == 1) ? (std::min(std::max({ head.size.x, head.size.y, head.size.z }) * 1.0f, globals::visuals::options::head_size_limit)) : globals::visuals::options::head_static_size;
  					float vertical_offset = radius * sinf(IM_PI / 3.0f);
  					rbx::vector3_t hex_bottom = head.position - globals::get_camera().get_rotation() * rbx::vector3_t{ 0, vertical_offset, 0 };
  					draw_polyline_w({ hex_bottom, shoulder_center, hip_center });
  				}
				else
				{
					draw_polyline_w({ head.position, head_bottom, shoulder_center, hip_center });
				}

				// Left Arm
				std::vector<rbx::vector3_t> left_arm = { shoulder_center, shoulder_left };
				if (entity.parts->count("LeftUpperArm")) left_arm.push_back((*entity.parts)["LeftUpperArm"].position);
				if (entity.parts->count("LeftLowerArm")) left_arm.push_back((*entity.parts)["LeftLowerArm"].position);
				if (entity.parts->count("LeftHand")) left_arm.push_back((*entity.parts)["LeftHand"].position);
				draw_polyline_w(left_arm);

				// Right Arm
				std::vector<rbx::vector3_t> right_arm = { shoulder_center, shoulder_right };
				if (entity.parts->count("RightUpperArm")) right_arm.push_back((*entity.parts)["RightUpperArm"].position);
				if (entity.parts->count("RightLowerArm")) right_arm.push_back((*entity.parts)["RightLowerArm"].position);
				if (entity.parts->count("RightHand")) right_arm.push_back((*entity.parts)["RightHand"].position);
				draw_polyline_w(right_arm);

				// Left Leg
				std::vector<rbx::vector3_t> left_leg = { hip_center };
				if (entity.parts->count("LeftUpperLeg")) left_leg.push_back((*entity.parts)["LeftUpperLeg"].position);
				if (entity.parts->count("LeftLowerLeg")) left_leg.push_back((*entity.parts)["LeftLowerLeg"].position);
				if (entity.parts->count("LeftFoot")) left_leg.push_back((*entity.parts)["LeftFoot"].position);
				draw_polyline_w(left_leg);

				// Right Leg
				std::vector<rbx::vector3_t> right_leg = { hip_center };
				if (entity.parts->count("RightUpperLeg")) right_leg.push_back((*entity.parts)["RightUpperLeg"].position);
				if (entity.parts->count("RightLowerLeg")) right_leg.push_back((*entity.parts)["RightLowerLeg"].position);
				if (entity.parts->count("RightFoot")) right_leg.push_back((*entity.parts)["RightFoot"].position);
				draw_polyline_w(right_leg);
			}
		}
	}

	static void tracers(rbx::vector3_t world_pos, ImU32 color, float thickness = 1.0f, int origin_type = 0, rbx::vector2_t dims = {}, rbx::matrix4_t view = {}, ImVec2 window_offset = {})
	{
		rbx::vector2_t screen = globals::visualengine.world_to_screen(world_pos, dims, view);

		if (screen.x != -1 && screen.y != -1)
		{
			ImVec2 target = { screen.x + window_offset.x, screen.y + window_offset.y };
			ImVec2 start;

			switch (origin_type)
			{
			case 0:
				start = { window_offset.x + dims.x * 0.5f, window_offset.y + dims.y };
				break;
			case 1:
				start = { window_offset.x + dims.x * 0.5f, window_offset.y };
				break;
			case 2:
				start = { window_offset.x + dims.x * 0.5f, window_offset.y + dims.y * 0.5f };
				break;
			case 3:
			{
				POINT p;
				if (GetCursorPos(&p))
					start = { (float)p.x, (float)p.y };
				else
					start = ImGui::GetIO().MousePos;
				break;
			}
			default:
				start = { window_offset.x + dims.x * 0.5f, window_offset.y + dims.y };
				break;
			}

			auto draw_list = ImGui::GetBackgroundDrawList();

			if (should_render_outline(2))
			{
				draw_list->AddLine(start, target, IM_COL32(0, 0, 0, 255), thickness + 2.0f);
			}

			draw_list->AddLine(start, target, color, thickness);
		}
	}

	static void fov_circle(rbx::vector2_t dims = {}, ImU32 color = 0)
	{
		if (!globals::visuals::fov_circle::enabled) return;

		auto draw_list = ImGui::GetBackgroundDrawList();
		ImVec2 center;

		if (globals::visuals::fov_circle::origin == 2)
		{
			if (globals::aimbot::has_target)
				center = { globals::aimbot::target_screen_pos.x, globals::aimbot::target_screen_pos.y };
			else
				center = ImGui::GetIO().MousePos;
		}
		else if (globals::visuals::fov_circle::origin == 1)
		{
			center = ImGui::GetIO().MousePos;
		}
		else
		{
			center = ImVec2(dims.x * 0.5f, dims.y * 0.5f);
		}

		float radius = globals::aimbot::fov * (dims.x / 100.0f);
		float thickness = globals::visuals::fov_circle::thickness;
		int sides = globals::visuals::fov_circle::num_sides;
		ImU32 final_color = color != 0 ? color : globals::visuals::fov_circle::color;

		if (globals::visuals::fov_circle::rainbow)
		{
			float t = (float)ImGui::GetTime() * globals::visuals::fov_circle::rainbow_speed;
			final_color = ImGui::ColorConvertFloat4ToU32(ImVec4(
				(sinf(t) + 1.0f) * 0.5f,
				(sinf(t + 2.0f) + 1.0f) * 0.5f,
				(sinf(t + 4.0f) + 1.0f) * 0.5f,
				1.0f
			));
		}

		if (globals::visuals::fov_circle::filled)
		{
			draw_list->AddCircleFilled(center, radius, globals::visuals::fov_circle::fill_color, sides);
		}

		draw_list->AddCircle(center, radius, final_color, sides, thickness);
	}

	static void crosshair(rbx::vector2_t dims = {}, ImU32 color = 0)
	{
		if (!globals::visuals::crosshair::enabled) return;



		if (globals::visuals::crosshair::sniper_crosshair) {

		}

		auto draw_list = ImGui::GetBackgroundDrawList();
		ImVec2 center;

		if (globals::visuals::crosshair::origin == 2)
		{
			if (globals::aimbot::has_target)
				center = { globals::aimbot::target_screen_pos.x, globals::aimbot::target_screen_pos.y };
			else
				center = ImGui::GetIO().MousePos;
		}
		else if (globals::visuals::crosshair::origin == 1)
		{
			center = ImGui::GetIO().MousePos;
		}
		else
		{
			center = ImVec2(dims.x * 0.5f, dims.y * 0.5f);
		}

		static ImVec2 lerped_center = center;
		if (globals::visuals::crosshair::lerp)
		{
			lerped_center = ImLerp(lerped_center, center, ImGui::GetIO().DeltaTime * globals::visuals::crosshair::lerp_speed);
		}
		else
		{
			lerped_center = center;
		}

		float time = (float)ImGui::GetTime();
		float angle = globals::visuals::crosshair::spin ? time * globals::visuals::crosshair::spin_speed * 5.0f : 0.0f;
		float pulse_val = globals::visuals::crosshair::pulse ? (sinf(time * globals::visuals::crosshair::pulse_speed * 5.0f) + 1.0f) * 0.5f : 0.0f;

		float size = globals::visuals::crosshair::size;
		float thickness = globals::visuals::crosshair::thickness;
		float gap = globals::visuals::crosshair::gap + (pulse_val * 10.0f); // Adjust 10.0f as needed for pulse range

		ImU32 final_color = color != 0 ? color : globals::visuals::crosshair::color;
		if (globals::visuals::crosshair::rainbow)
		{
			float t = time * globals::visuals::crosshair::rainbow_speed;
			final_color = ImGui::ColorConvertFloat4ToU32(ImVec4(
				(sinf(t) + 1.0f) * 0.5f,
				(sinf(t + 2.0f) + 1.0f) * 0.5f,
				(sinf(t + 4.0f) + 1.0f) * 0.5f,
				1.0f
			));
		}

		auto draw_line = [&](ImVec2 start, ImVec2 end, ImU32 col, float thick) {
			if (globals::visuals::crosshair::outline)
				draw_list->AddLine(start, end, globals::visuals::crosshair::outline_color, thick + (globals::visuals::crosshair::outline_thickness * 2.0f));
			draw_list->AddLine(start, end, col, thick);
		};

		if (globals::visuals::crosshair::dot)
		{
			if (globals::visuals::crosshair::dot_outline)
				draw_list->AddCircleFilled(lerped_center, globals::visuals::crosshair::dot_size + globals::visuals::crosshair::outline_thickness, globals::visuals::crosshair::outline_color);
			draw_list->AddCircleFilled(lerped_center, globals::visuals::crosshair::dot_size, globals::visuals::crosshair::dot_color);
		}

		auto draw_rotated_line = [&](float start_angle, float length) {
			float rad = (angle + start_angle) * (3.14159265f / 180.0f);
			ImVec2 start = { lerped_center.x + cosf(rad) * gap, lerped_center.y + sinf(rad) * gap };
			ImVec2 end = { lerped_center.x + cosf(rad) * (gap + length), lerped_center.y + sinf(rad) * (gap + length) };
			draw_line(start, end, final_color, thickness);
		};

		draw_rotated_line(0.0f, size);
		draw_rotated_line(180.0f, size);
		draw_rotated_line(90.0f, size);
		if (!globals::visuals::crosshair::t_style)
			draw_rotated_line(270.0f, size);
	}
}

