#pragma once
#include "../../../dependencies/common_includes.hpp"
#include "../../../dependencies/math/math.hpp"

struct stored_records {
	vec3_t head;
	int flags;
	float simulation_time;
	matrix_t matrix[128];
};

struct convars {
	convar* updateRate;
	convar* maxUpdateRate;
	convar* interp;
	convar* interpRatio;
	convar* minInterpRatio;
	convar* maxInterpRatio;
	convar* maxUnlag;
};

extern std::deque<stored_records> records[65];
extern convars cvars;

class c_backtrack : public singleton< c_backtrack > {
public:
	void update();
	void run(c_usercmd*);
	float get_lerp_time();
	int time_to_ticks(float time);
	bool valid_tick(float simtime);
	static void init() {
		records->clear();

		cvars.updateRate = interfaces::console->get_convar("cl_updaterate");
		cvars.maxUpdateRate = interfaces::console->get_convar("sv_maxupdaterate");
		cvars.interp = interfaces::console->get_convar("cl_interp");
		cvars.interpRatio = interfaces::console->get_convar("cl_interp_ratio");
		cvars.minInterpRatio = interfaces::console->get_convar("sv_client_min_interp_ratio");
		cvars.maxInterpRatio = interfaces::console->get_convar("sv_client_max_interp_ratio");
		cvars.maxUnlag = interfaces::console->get_convar("sv_maxunlag");
	}
};