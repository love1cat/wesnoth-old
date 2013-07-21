/*
 Copyright (C) 2009 - 2013 by Yuan Song <yuan.uconn@gmail.com>
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
 * power_projection: multi / improved version
 */

#ifndef AI_TESTING_MULTI_POWER_PROJECTION_HPP
#define AI_TESTING_MULTI_POWER_PROJECTION_HPP

#include "target_analysis_strategy.hpp"

class unit;

namespace ai {

namespace testing_ai_default {
	
class aspect_attacks;
	
class multi_power_projection_strategy : public target_analysis_strategy1 {
	multi_power_projection_strategy(const config& cfg, const aspect_attacks *aspect_attacks_ptr);
	virtual double power_projection(const std::vector<map_location>& locs, const move_map& dstsrc) const;
	virtual int rate_unit_on_tile(const unit& un, const map_location& tile) const;
};
	
} // end of namespace testing_ai_default
	
} // end of namespace ai

#endif /* defined(AI_TESTING_MULTI_POWER_PROJECTION_HPP) */
