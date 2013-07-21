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
 * @file
 * analysis_strategy: target
 */

#ifndef AI_TESTING_TARGET_ANALYSIS_STRATEGY_HPP
#define AI_TESTING_TARGET_ANALYSIS_STRATEGY_HPP

#include <vector>
#include "../interface.hpp"
#include "analysis_strategy.hpp"

namespace ai {

namespace testing_ai_default {

class aspect_attacks;

class target_analysis_strategy : public analysis_strategy {
public:
	virtual boost::shared_ptr<attacks_vector> analyze_targets_impl() const = 0;
};

class target_analysis_strategy1 : public target_analysis_strategy {
public:
	target_analysis_strategy1(const config& cfg, const aspect_attacks *aspect_attacks_ptr);
	virtual boost::shared_ptr<attacks_vector> analyze_targets_impl() const;
protected:
	virtual double rate_terrain(const unit& u, const map_location& loc) const;

	virtual void do_attack_analysis(const map_location& loc,
									const move_map& srcdst,
									const move_map& dstsrc,
									const move_map& fullmove_srcdst,
									const move_map& fullmove_dstsrc,
									const move_map& enemy_srcdst,
									const move_map& enemy_dstsrc,
									const map_location* tiles,
									const team& current_team,
									bool* used_locations,
									std::vector<map_location>& units,
									std::vector<attack_analysis>& result,
									attack_analysis& cur_analysis) const;

	virtual void set_default_attack_analysis_strategy(const config& target_anaysis_strategy_config);
	virtual double power_projection(const std::vector<map_location>& locs, const move_map& dstsrc) const;

	config target_analysis_strategy_cfg_;
	config attack_analysis_strategy_cfg_;
	const aspect_attacks * aspect_attacks_ptr_;
};

} // end of namespace testing_ai_default

} // end of namespace ai

#endif
