#include "backtrack.hpp"

std::deque<stored_records> records[65];
convars cvars;

float c_backtrack::get_lerp_time() {
	auto ratio = std::clamp(cvars.interpRatio->get_float(), (cvars.minInterpRatio->get_float() != 1.f) ? cvars.minInterpRatio->get_float() : 1.f, cvars.maxInterpRatio->get_float());
	return max(cvars.interp->get_float(), (ratio / ((cvars.maxUpdateRate) ? cvars.maxUpdateRate->get_float() : cvars.updateRate->get_float())));
}

int c_backtrack::time_to_ticks(float time) {
	return static_cast<int>((0.5f + (float)(time) / interfaces::globals->interval_per_tick));
}

bool c_backtrack::valid_tick(float simtime) {
	auto network = interfaces::engine->get_net_channel_info();
	if (!network)
		return false;

	auto delta = std::clamp(network->get_latency(0) + get_lerp_time(), 0.f, cvars.maxUnlag->get_float()) - (interfaces::globals->cur_time - simtime);
	return (std::fabsf(delta) < 0.2f);
}

void c_backtrack::update() {
	auto local_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));
	if (!c_config::get().backtrack || !local_player || !local_player->is_alive()) {
		if (!records->empty())
			records->clear();

		return;
	}
	for (int i = 1; i <= interfaces::globals->max_clients; i++) {
		auto entity = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(i));
		if (!entity || entity == local_player || entity->dormant() || !entity->is_alive() || !entity->is_enemy()) {
			if (records[i].size() > 2)
				records[i].clear();

			continue;
		}

		auto& rec = records[i];
		for (auto r = rec.begin(); r != rec.end();) {
			if (!valid_tick(r->simulation_time))
				r = rec.erase(r);
			else
				r++;
		}

		if (records[i].size() && (records[i].front().simulation_time == entity->simulation_time()))
			continue;

		while (records[i].size() > 3 && records[i].size() > static_cast<size_t>(time_to_ticks(static_cast<float>(200) / 1000.f)))
			records[i].pop_back();

		auto varmap = reinterpret_cast<uintptr_t>(entity) + 0x24;
		auto varscount = *reinterpret_cast<int*>(static_cast<uintptr_t>(varmap) + 0x14);
		for (int j = 0; j < varscount; j++)
			* reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(varmap) + j * 0xC) = 0;

		stored_records record{ };
		record.head = entity->get_bone_position(8);
		record.flags = entity->flags();
		record.simulation_time = entity->simulation_time();

		entity->setup_bones(record.matrix, 128, 0x7FF00, interfaces::globals->cur_time);

		records[i].emplace_front(record);
	}
}

void c_backtrack::run(c_usercmd * cmd) {
	if (!c_config::get().backtrack)
		return;

	if (!(cmd->buttons & in_attack))
		return;

	auto local_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));
	if (!local_player)
		return;

	auto best_fov{ 255.f };
	player_t* best_target{ };
	int besst_target_index{ };
	vec3_t best_target_head_position{ };
	int best_record{ };

	for (int i = 1; i <= interfaces::globals->max_clients; i++) {
		auto entity = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(i));
		if (!entity || entity == local_player || entity->dormant() || !entity->is_alive()
			|| !entity->is_enemy())
			continue;

		auto head_position = entity->get_bone_position(8);

		auto angle = c_math::get().calculate_angle(local_player->get_eye_pos(), head_position, cmd->viewangles);
		auto fov = std::hypotf(angle.x, angle.y);
		if (fov < best_fov) {
			best_fov = fov;
			best_target = entity;
			besst_target_index = i;
			best_target_head_position = head_position;
		}
	}

	if (best_target) {
		if (records[besst_target_index].size() <= 3)
			return;

		best_fov = 255.f;

		for (size_t i = 0; i < records[besst_target_index].size(); i++) {
			auto record = &records[besst_target_index][i];
			if (!record || !valid_tick(record->simulation_time))
				continue;

			auto angle = c_math::get().calculate_angle(local_player->get_eye_pos(), record->head, cmd->viewangles);
			auto fov = std::hypotf(angle.x, angle.y);
			if (fov < best_fov) {
				best_fov = fov;
				best_record = i;
			}
		}
	}

	if (best_record) {
		auto record = records[besst_target_index][best_record];

		if (cmd->buttons & in_attack)
			cmd->tick_count = time_to_ticks(record.simulation_time);
	}
}