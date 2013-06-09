/*
   Copyright (C) 2009 - 2013 by Yurii Chernyi <terraninfo@terraninfo.net>
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
 * Stage: fallback to other AI
 * @file
 */

#include "aspect_attacks.hpp"

#include "../manager.hpp"
#include "../../actions/attack.hpp"
#include "../../log.hpp"
#include "../../map.hpp"
#include "../../team.hpp"
#include "../../tod_manager.hpp"
#include "../../resources.hpp"
#include "../../unit.hpp"
#include "../../pathfind/pathfind.hpp"

#include "target_analysis_strategy.hpp"
#include "multi_target_analysis_strategy.hpp"

namespace ai {

namespace testing_ai_default {

static lg::log_domain log_ai_testing_aspect_attacks("ai/aspect/attacks");
#define DBG_AI LOG_STREAM(debug, log_ai_testing_aspect_attacks)
#define LOG_AI LOG_STREAM(info, log_ai_testing_aspect_attacks)
#define ERR_AI LOG_STREAM(err, log_ai_testing_aspect_attacks)

aspect_attacks::aspect_attacks(readonly_context &context, const config &cfg, const std::string &id)
	: typesafe_aspect<attacks_vector>(context,cfg,id)
	, filter_own_()
	, filter_enemy_()
	, target_analysis_strategy_cfg_()
{
	if (const config &filter_own = cfg.child("filter_own")) {
		filter_own_ = filter_own;
	}
	if (const config &filter_enemy = cfg.child("filter_enemy")) {
		filter_enemy_ = filter_enemy;
	}
	if (const config &target_analysis_strategy_cfg = cfg.child("target_analysis_strategy")){
		target_analysis_strategy_cfg_ = target_analysis_strategy_cfg;
		set_strategy_from_config(target_analysis_strategy_cfg);
	}
}

aspect_attacks::~aspect_attacks()
{
}

void aspect_attacks::recalculate() const
{
	this->value_ = analyze_targets();
	this->valid_ = true;
}

boost::shared_ptr<attacks_vector> aspect_attacks::analyze_targets() const
{
	LOG_AI<<"current target analysis strategy is: "<<get_strategy()->get_id()<<std::endl;
	return get_strategy()->analyze_targets_impl(*this);
}


config aspect_attacks::to_config() const
{
	config cfg = typesafe_aspect<attacks_vector>::to_config();
	if (!filter_own_.empty()) {
		cfg.add_child("filter_own",filter_own_);
	}
	if (!filter_enemy_.empty()) {
		cfg.add_child("filter_enemy",filter_enemy_);
	}
	if (!target_analysis_strategy_cfg_.empty()) {
		cfg.add_child("target_analysis_strategy",target_analysis_strategy_cfg_);
	}
	return cfg;
}
	
aspect_attacks::target_analysis_strategy_ptr aspect_attacks::get_default_strategy() const{
	LOG_AI << "default target analysis strategy is applied" << std::endl;
	// set default analysis strategy
	return target_analysis_strategy_ptr(new target_analysis_strategy1(target_analysis_strategy_cfg_));
}

void aspect_attacks::set_strategy_from_config(const config& target_analysis_strategy_cfg){
	// set target analysis strategy according to the strategy name from the config
	if(target_analysis_strategy_cfg.has_attribute("name")){
		std::string stra_name = target_analysis_strategy_cfg["name"];
		if(stra_name == "strategy1"){
			target_analysis_strategy_ptr stra_ptr= target_analysis_strategy_ptr(new target_analysis_strategy1(target_analysis_strategy_cfg_));
			set_current_strategy(stra_ptr);
			LOG_AI << "target_analysis_strategy is set to " << get_current_strategy()->get_id() << " by config" << std::endl;
		} //TODO: add new strategy (with different name attribute) in "else if" block
		else if(stra_name == "multi_target") {
			target_analysis_strategy_ptr stra_ptr= target_analysis_strategy_ptr(new multi_target_analysis_strategy(target_analysis_strategy_cfg_));
			set_current_strategy(stra_ptr);
			LOG_AI << "target_analysis_strategy is set to " << get_current_strategy()->get_id() << " by config" << std::endl;
		}
		else{
			ERR_AI << "target_analysis_strategy tag has invalid 'name' attribute " << stra_name << std::endl;
		}
	}
}

} // end of namespace testing_ai_default

} // end of namespace ai
