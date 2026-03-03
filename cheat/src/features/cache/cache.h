#pragma once
#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <memory>
#include "../sdk/sdk.h"

namespace cache
{
	inline std::mutex mtx;

	struct part_data
	{
		rbx::part_t part;
		std::uint64_t primitive;
		std::string name;
		rbx::vector3_t position;
		rbx::vector3_t size;
		rbx::vector3_t scale;
		rbx::matrix3_t rotation;
		std::string mesh_id;
	};

	struct entity_t final
	{
		rbx::instance_t instance;
		std::string name;
		std::string display_name;
		std::string tool_name;
		float health;
		float maxhealth;
		float distance = 0.f;
		std::uint64_t team_address = 0;
		rbx::vector3_t team_color = { 1.f, 1.f, 1.f };

		int custom_team = 0; // 0: none, 1: terrorist, 2: counter-terrorist

		std::uint8_t rig_type;

		rbx::humanoid_t humanoid;

		part_data root_part;
		part_data head;

		rbx::vector3_t velocity;

		std::shared_ptr<std::unordered_map<std::string, part_data>> parts = std::make_shared<std::unordered_map<std::string, part_data>>();
	};

	inline cache::entity_t cached_local_player;
	inline std::uint64_t local_player_team = 0;
	inline std::shared_ptr<std::vector<cache::entity_t>> cached_players = std::make_shared<std::vector<cache::entity_t>>();

	void run();
	void tphandler();
}

