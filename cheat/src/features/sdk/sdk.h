#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "math/math.h"
#include "memory/memory.h"
#include "offsets/offsets.h"

namespace rbx
{
	class instance_t;
	class primitive_t;
	class model_instance_t;

	struct addressable_t
	{
		std::uint64_t address;

		addressable_t() : address(0) {}
		addressable_t(std::uint64_t address) : address(address) {}
	};

	struct nameable_t : public addressable_t
	{
		using addressable_t::addressable_t;

		std::string get_name();
		std::string get_class_name();
	};

	struct interface_t
	{
		template <typename T>
		std::vector<T> get_children();

		std::vector<rbx::instance_t> get_children();
		bool has_children();
		rbx::instance_t find_first_child(std::string_view str);
		rbx::instance_t find_first_child_by_class(std::string_view str);
	};

	struct instance_t : public nameable_t, public interface_t
	{
		using nameable_t::nameable_t;
	};

	struct player_t final : public instance_t
	{
		using instance_t::instance_t;

		rbx::model_instance_t get_model_instance();
		rbx::instance_t get_team();
		std::string get_display_name();
	};

	struct model_instance_t final : public instance_t
	{
		using instance_t::instance_t;
	};

	struct team_t final : public instance_t
	{
		using instance_t::instance_t;

		rbx::vector3_t get_team_color();
	};

	struct humanoid_t final : public addressable_t
	{
		using addressable_t::addressable_t;

		std::uint8_t get_rig_type();
		std::uint8_t get_state();
		float get_health() const;
		float get_max_health() const;
	};

	struct part_t : public instance_t
	{
		using instance_t::instance_t;

		rbx::primitive_t get_primitive();
		rbx::vector3_t get_part_position();
		void set_part_position(rbx::vector3_t position);
	};

	struct mesh_part_t final : public part_t
	{
		using part_t::part_t;
		std::string get_mesh_id();
	};

	struct special_mesh_t final : public instance_t
	{
		using instance_t::instance_t;
		std::string get_mesh_id();
		rbx::vector3_t get_scale();
	};

	struct primitive_t final : public addressable_t
	{
		using addressable_t::addressable_t;

		rbx::vector3_t get_size();
		rbx::vector3_t get_position();
		rbx::matrix3_t get_rotation();
	};

	struct visualengine_t final : public addressable_t
	{
		rbx::vector2_t get_dimensions();
		rbx::matrix4_t get_viewmatrix();
		rbx::vector2_t world_to_screen(rbx::vector3_t world, rbx::vector2_t dimensions, rbx::matrix4_t viewmatrix);
	};

	struct camera_t final : public instance_t
	{
		using instance_t::instance_t;

		rbx::vector3_t get_position();
		void set_position(rbx::vector3_t position);

		rbx::matrix3_t get_rotation();
		void set_rotation(rbx::matrix3_t rotation);
	};
}

template <typename T>
std::vector<T> rbx::interface_t::get_children()
{
	rbx::instance_t* base = static_cast<rbx::instance_t*>(this);

	std::uint64_t start = memory->read<std::uint64_t>(base->address + Offsets::Instance::ChildrenStart);
	std::uint64_t end = memory->read<std::uint64_t>(start + Offsets::Instance::ChildrenEnd);

	std::vector<T> children;
	children.reserve(32);

	for (std::uint64_t instance = memory->read<std::uint64_t>(start); instance != end; instance += sizeof(std::shared_ptr<void*>))
	{
		children.emplace_back(memory->read<std::uint64_t>(instance));
	}

	return children;
}

inline bool rbx::interface_t::has_children()
{
	rbx::instance_t* base = static_cast<rbx::instance_t*>(this);
	if (!base || !base->address) return false;

	std::uint64_t start = memory->read<std::uint64_t>(base->address + Offsets::Instance::ChildrenStart);
	if (!start) return false;

	std::uint64_t end = memory->read<std::uint64_t>(start + Offsets::Instance::ChildrenEnd);
	return start != 0 && end != 0 && end > start;
}

