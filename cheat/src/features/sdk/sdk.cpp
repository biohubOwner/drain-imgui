#include "sdk.h"
#include "offsets/offsets.h"
#include "math/math.h"

std::string rbx::nameable_t::get_name()
{
	if (!memory->is_valid_address(this->address)) return "unknown";
	std::uint64_t name = memory->read<std::uint64_t>(this->address + Offsets::Instance::Name);

	if (memory->is_valid_address(name))
	{
		return memory->read_string(name);
	}

	return "unknown";
}

std::string rbx::nameable_t::get_class_name()
{
	if (!memory->is_valid_address(this->address)) return "unknown";
	std::uint64_t class_descriptor = memory->read<std::uint64_t>(this->address + Offsets::Instance::ClassDescriptor);
	if (!memory->is_valid_address(class_descriptor)) return "unknown";

	std::uint64_t class_name = memory->read<std::uint64_t>(class_descriptor + Offsets::Instance::ClassName);

	if (memory->is_valid_address(class_name))
	{
		return memory->read_string(class_name);
	}

	return "unknown";
}
std::vector<rbx::instance_t> rbx::interface_t::get_children()
{
	rbx::instance_t* base = static_cast<rbx::instance_t*>(this);
    if (!base->address) return {};

	std::uint64_t start{ memory->read<std::uint64_t>(base->address + Offsets::Instance::ChildrenStart) };
    if (!memory->is_valid_address(start)) return {};

	std::uint64_t end{ memory->read<std::uint64_t>(start + Offsets::Instance::ChildrenEnd) };
    if (!memory->is_valid_address(end)) return {};

	std::vector<rbx::instance_t> children;
	children.reserve(32);

    int count = 0;
	for (std::uint64_t instance = memory->read<std::uint64_t>(start); instance != end && count < 100000; instance += sizeof(std::shared_ptr<void*>), count++)
	{
        std::uint64_t child_addr = memory->read<std::uint64_t>(instance);
        if (memory->is_valid_address(child_addr))
		    children.emplace_back(child_addr);
	}

	return children;
}

rbx::instance_t rbx::interface_t::find_first_child(std::string_view str)
{
	std::vector<rbx::instance_t> children = this->get_children();

	for (rbx::instance_t& child : children)
	{
		if (child.get_name() == str)
		{
			return child;
		}
	}

	return {};
}

rbx::instance_t rbx::interface_t::find_first_child_by_class(std::string_view str)
{
	std::vector<rbx::instance_t> children = this->get_children();

	for (rbx::instance_t& child : children)
	{
		if (child.get_class_name() == str)
		{
			return child;
		}
	}

	return {};
}

rbx::model_instance_t rbx::player_t::get_model_instance()
{
    if (!memory->is_valid_address(this->address)) return { 0 };
	return { memory->read<std::uint64_t>(this->address + Offsets::Player::ModelInstance) };
}

rbx::instance_t rbx::player_t::get_team()
{
    if (!memory->is_valid_address(this->address)) return { 0 };
	return { memory->read<std::uint64_t>(this->address + Offsets::Player::Team) };
}

rbx::vector3_t rbx::team_t::get_team_color()
{
	if (!memory->is_valid_address(this->address)) return { 1.f, 1.f, 1.f };
	return memory->read<rbx::vector3_t>(this->address + Offsets::Team::BrickColor);
}

std::string rbx::player_t::get_display_name()
{
	if (!memory->is_valid_address(this->address)) return "unknown";
	std::uint64_t display_name = memory->read<std::uint64_t>(this->address + Offsets::Player::DisplayName);

	if (memory->is_valid_address(display_name))
	{
		return memory->read_string(display_name);
	}

	return "unknown";
}

std::uint8_t rbx::humanoid_t::get_rig_type()
{
    if (!memory->is_valid_address(this->address)) return 0;
	return memory->read<std::uint8_t>(this->address + Offsets::Humanoid::RigType);
}

std::uint8_t rbx::humanoid_t::get_state()
{
    if (!memory->is_valid_address(this->address)) return 0;
	return memory->read<std::uint8_t>(this->address + Offsets::Humanoid::State);
}

float rbx::humanoid_t::get_max_health() const
{
	if (!memory->is_valid_address(this->address)) return 0.f;
	return memory->read<float>(this->address + Offsets::Humanoid::MaxHealth);
}

float rbx::humanoid_t::get_health() const
{
	if (!memory->is_valid_address(this->address)) return 0.f;
	return memory->read<float>(this->address + Offsets::Humanoid::Health);
}

rbx::primitive_t rbx::part_t::get_primitive()
{
    if (!memory->is_valid_address(this->address)) return { 0 };
	return { memory->read<std::uint64_t>(this->address + Offsets::BasePart::Primitive) };
}

rbx::vector3_t rbx::part_t::get_part_position()
{
	std::uint64_t primitive = this->get_primitive().address;
    if (!memory->is_valid_address(primitive)) return { 0, 0, 0 };

	return memory->read<rbx::vector3_t>(primitive + Offsets::BasePart::Position);
}

void rbx::part_t::set_part_position(rbx::vector3_t position)
{
	std::uint64_t primitive = this->get_primitive().address;
    if (!memory->is_valid_address(primitive)) return;

	memory->write<rbx::vector3_t>(primitive + Offsets::BasePart::Position, position);
}

std::string rbx::mesh_part_t::get_mesh_id()
{
	if (!memory->is_valid_address(this->address)) return "unknown";
	std::uint64_t mesh_id = memory->read<std::uint64_t>(this->address + Offsets::MeshPart::MeshID_Offset);

	if (memory->is_valid_address(mesh_id))
	{
		return memory->read_string(mesh_id);
	}

	return "unknown";
}

std::string rbx::special_mesh_t::get_mesh_id()
{
	if (!memory->is_valid_address(this->address)) return "unknown";
	std::uint64_t mesh_id = memory->read<std::uint64_t>(this->address + Offsets::SpecialMesh::MeshID_Offset);

	if (memory->is_valid_address(mesh_id))
	{
		return memory->read_string(mesh_id);
	}

	return "unknown";
}

rbx::vector3_t rbx::special_mesh_t::get_scale()
{
    if (!memory->is_valid_address(this->address)) return { 1, 1, 1 };
	return memory->read<rbx::vector3_t>(this->address + Offsets::SpecialMesh::Scale);
}

rbx::vector3_t rbx::primitive_t::get_size()
{
    if (!memory->is_valid_address(this->address)) return { 0, 0, 0 };
	return memory->read<rbx::vector3_t>(this->address + Offsets::BasePart::Size);
}

rbx::vector3_t rbx::primitive_t::get_position()
{
    if (!memory->is_valid_address(this->address)) return { 0, 0, 0 };
	return memory->read<rbx::vector3_t>(this->address + Offsets::BasePart::Position);
}

rbx::matrix3_t rbx::primitive_t::get_rotation()
{
    if (!memory->is_valid_address(this->address)) return {};
	return memory->read<rbx::matrix3_t>(this->address + Offsets::BasePart::Rotation);
}

rbx::vector2_t rbx::visualengine_t::get_dimensions()
{
    if (!memory->is_valid_address(this->address)) return { 0, 0 };
	return memory->read<rbx::vector2_t>(this->address + Offsets::VisualEngine::Dimensions);
}

rbx::matrix4_t rbx::visualengine_t::get_viewmatrix()
{
    if (!memory->is_valid_address(this->address)) return {};
	return memory->read<rbx::matrix4_t>(this->address + Offsets::VisualEngine::ViewMatrix);
}

rbx::vector2_t rbx::visualengine_t::world_to_screen(rbx::vector3_t world, rbx::vector2_t dimensions, rbx::matrix4_t viewmatrix)
{
	rbx::vector4_t clipCoords = {
		world.x * viewmatrix(0, 0) + world.y * viewmatrix(0, 1) + world.z * viewmatrix(0, 2) + viewmatrix(0, 3),
		world.x * viewmatrix(1, 0) + world.y * viewmatrix(1, 1) + world.z * viewmatrix(1, 2) + viewmatrix(1, 3),
		world.x * viewmatrix(2, 0) + world.y * viewmatrix(2, 1) + world.z * viewmatrix(2, 2) + viewmatrix(2, 3),
		world.x * viewmatrix(3, 0) + world.y * viewmatrix(3, 1) + world.z * viewmatrix(3, 2) + viewmatrix(3, 3)
	};

	if (clipCoords.w < 0.1f)
		return { -1, -1 };

	float inv_w = 1.0f / clipCoords.w;
	rbx::vector3_t ndc = { clipCoords.x * inv_w, clipCoords.y * inv_w, clipCoords.z * inv_w };

	return {
		(dimensions.x / 2.0f) + (ndc.x * dimensions.x / 2.0f),
		(dimensions.y / 2.0f) - (ndc.y * dimensions.y / 2.0f)
	};
}

rbx::vector3_t rbx::camera_t::get_position()
{
	if (!memory->is_valid_address(this->address)) return { 0, 0, 0 };
	return memory->read<rbx::vector3_t>(this->address + Offsets::Camera::Position);
}

void rbx::camera_t::set_position(rbx::vector3_t position)
{
	if (!memory->is_valid_address(this->address)) return;
	memory->write<rbx::vector3_t>(this->address + Offsets::Camera::Position, position);
}

rbx::matrix3_t rbx::camera_t::get_rotation()
{
	if (!memory->is_valid_address(this->address)) return {};
	return memory->read<rbx::matrix3_t>(this->address + Offsets::Camera::Rotation);
}

void rbx::camera_t::set_rotation(rbx::matrix3_t rotation)
{
	if (!memory->is_valid_address(this->address)) return;
	memory->write<rbx::matrix3_t>(this->address + Offsets::Camera::Rotation, rotation);
}

