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
 * analysis_strategy: attack
 */

#ifndef AI_TESTING_ATTACK_ANALYSIS_STRATEGY_HPP
#define AI_TESTING_ATTACK_ANALYSIS_STRATEGY_HPP

#include "../default/contexts.hpp"
#include "analysis_strategy.hpp"

namespace ai {
	
namespace testing_ai_default {
	
class attack_analysis_strategy : public analysis_strategy {
public:
	virtual void analyze_impl(attack_analysis& aas,
							  const gamemap& map, 
							  unit_map& units,
							  const readonly_context& ai_obj,
							  const move_map& dstsrc, 
							  const move_map& srcdst,
							  const move_map& enemy_dstsrc, 
							  double aggression) const = 0;
	
	virtual double rating_impl(const attack_analysis& aas, double aggression, const readonly_context& ai_obj) const = 0;
};

class attack_analysis_strategy1 : public attack_analysis_strategy { 
public:
	virtual void analyze_impl(attack_analysis& aas,
							  const gamemap& map, 
							  unit_map& units,
							  const readonly_context& ai_obj,
							  const move_map& dstsrc, 
							  const move_map& srcdst,
							  const move_map& enemy_dstsrc, 
							  double aggression) const;
	
	virtual double rating_impl(const attack_analysis& aas, double aggression, const readonly_context& ai_obj) const;
};
	
} // end of namespace testing_ai_default
	
} // end of namespace ai

#endif
