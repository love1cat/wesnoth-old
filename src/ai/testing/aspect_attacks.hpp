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
 * Aspect: attacks
 * @file
 */

#ifndef AI_TESTING_ASPECT_ATTACKS_HPP_INCLUDED
#define AI_TESTING_ASPECT_ATTACKS_HPP_INCLUDED

#include "../composite/aspect.hpp"
#include "../interface.hpp"

#include "target_analysis_strategy.hpp"

#ifdef _MSC_VER
#pragma warning(push)
//silence "inherits via dominance" warnings
#pragma warning(disable:4250)
#endif

namespace ai {


namespace testing_ai_default {

class aspect_attacks: public typesafe_aspect<attacks_vector>, public virtual analysis_strategy_component<target_analysis_strategy> {
public:
	typedef analysis_strategy_component<target_analysis_strategy>::strategy_ptr target_analysis_strategy_ptr;
	
	aspect_attacks(readonly_context &context, const config &cfg, const std::string &id);


	virtual ~aspect_attacks();


	virtual void recalculate() const;


	virtual config to_config() const;
	
	inline const config& get_filter_own() const {return filter_own_;}
	inline const config& get_filter_enemy() const {return filter_enemy_;}
	inline const config& get_analysis_strategy_cfg() const {return target_analysis_strategy_cfg_;}
	
	virtual target_analysis_strategy_ptr get_default_strategy() const;
	
	virtual void set_strategy_from_config(const config& target_analysis_strategy_cfg);

protected:
	boost::shared_ptr<attacks_vector> analyze_targets() const;
	
	config filter_own_;
	config filter_enemy_;
	config target_analysis_strategy_cfg_;
};



} // end of namespace testing_ai_default

} // end of namespace ai

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
