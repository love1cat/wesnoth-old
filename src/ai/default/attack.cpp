/*
   Copyright (C) 2003 - 2013 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/**
 * @file
 * Calculate & analyze attacks of the default ai
 */

#include "../../global.hpp"

#include "ai.hpp"
#include "../manager.hpp"

#include "../../actions/attack.hpp"
#include "../../attack_prediction.hpp"
#include "../../game_config.hpp"
#include "../../log.hpp"
#include "../../map.hpp"
#include "../../team.hpp"

#include "../testing/multi_target_attack_analysis_strategy.hpp"

static lg::log_domain log_ai("ai/attack");
#define LOG_AI LOG_STREAM(info, log_ai)
#define ERR_AI LOG_STREAM(err, log_ai)

namespace ai {
	
attack_analysis::attack_analysis(const config& cfg, int target_number) :
	game_logic::formula_callable(),
	target(),
	movements(),
	target_value(0.0),
	avg_losses(0.0),
	chance_to_kill(0.0),
	avg_damage_inflicted(0.0),
	target_starting_damage(0),
	avg_damage_taken(0.0),
	resources_used(0.0),
	terrain_quality(0.0),
	alternative_terrain_quality(0.0),
	vulnerability(0.0),
	support(0.0),
	leader_threat(false),
	uses_leader(false),
	is_surrounded(false),
	analysis_results(target_number)
{
	if(const config &attack_analysis_strategy_cfg = cfg.child("attack_analysis_strategy")){
		set_strategy_from_config(attack_analysis_strategy_cfg);
	}
}

void attack_analysis::analyze(const gamemap& map, unit_map& units,
				  const readonly_context& ai_obj,
								  const move_map& dstsrc, const move_map& srcdst,
								  const move_map& enemy_dstsrc, double aggression)
{
	get_strategy()->analyze_impl(*this,
								 map,
								 units,
								 ai_obj,
								 dstsrc,
								 srcdst,
								 enemy_dstsrc,
								 aggression);
}

bool attack_analysis::attack_close(const map_location& loc) const
{
	std::set<map_location> &attacks = manager::get_ai_info().recent_attacks;
	for(std::set<map_location>::const_iterator i = attacks.begin(); i != attacks.end(); ++i) {
		if(distance_between(*i,loc) < 4) {
			return true;
		}
	}

	return false;
}


double attack_analysis::rating(double aggression, const readonly_context& ai_obj) const
{
	return get_strategy()->rating_impl(*this, aggression, ai_obj);
}
	
attack_analysis::attack_analysis_strategy_ptr attack_analysis::get_default_strategy() const{
	LOG_AI << "default attack analysis strategy is applied" << std::endl;
	return attack_analysis::attack_analysis_strategy_ptr(new testing_ai_default::attack_analysis_strategy1());
}

void attack_analysis::set_strategy_from_config(const config& attack_analysis_strategy_cfg){
	// set attack analysis strategy according to the strategy name from the config
	if(attack_analysis_strategy_cfg.has_attribute("name")){
		std::string stra_name = attack_analysis_strategy_cfg["name"];
		if(stra_name == "strategy1"){
			attack_analysis_strategy_ptr stra_ptr= attack_analysis_strategy_ptr(new testing_ai_default::attack_analysis_strategy1());
			set_current_strategy(stra_ptr);
			LOG_AI << "attack_analysis_strategy is set to " << get_current_strategy()->get_id()  << " by config" << std::endl;
		} else if(stra_name == "multi_target"){
			attack_analysis_strategy_ptr stra_ptr= attack_analysis_strategy_ptr(new testing_ai_default::multi_target_attack_analysis_strategy());
			set_current_strategy(stra_ptr);
			LOG_AI << "attack_analysis_strategy is set to " << get_current_strategy()->get_id()  << " by config" << std::endl;
		}
		// TODO: add new strategy (with different name attribute) in "else if" block
		else {
			ERR_AI << "attack_analysis_strategy tag has invalid 'name' attribute " << stra_name << std::endl;
		}
	}   
}

} //end of namespace ai
