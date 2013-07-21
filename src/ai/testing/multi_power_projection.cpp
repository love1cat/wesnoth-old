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

#include <boost/foreach.hpp>


#include "../../resources.hpp"
#include "../../tod_manager.hpp"
#include "../../map.hpp"
#include "../../unit_map.hpp"
#include "../../unit.hpp"
#include "aspect_attacks.hpp"

#include "multi_power_projection.hpp"
#include "max_weight_match.hpp"


namespace ai {

namespace testing_ai_default {

multi_power_projection_strategy::multi_power_projection_strategy(const config& cfg, const aspect_attacks *aspect_attacks_ptr)
	: target_analysis_strategy1(cfg, aspect_attacks_ptr) {}

double multi_power_projection_strategy::power_projection(const std::vector<map_location>& locs, const move_map& dstsrc) const {

	gamemap& map = *resources::game_map;
	unit_map& units = *resources::units;

	// Find adjacent tiles of given locations.
	// Use set to avoid overlapped tiles
	std::set<map_location> adj_tiles;
	BOOST_FOREACH(map_location loc, locs) {
		map_location adj_locs[6];
		get_adjacent_tiles(loc, adj_locs);
		BOOST_FOREACH(map_location adj_loc, adj_locs) {
			if (map.on_board(adj_loc)) {
				adj_tiles.insert(adj_loc);
			}
		}
	}

	// Index the units that can reach these adjacent tiles.
	// Use map to avoid duplicate unit.
	std::map<map_location, int> unit_id;
	typedef std::set<map_location>::const_iterator loc_iter;
	typedef move_map::const_iterator movemap_iter;
	typedef std::pair<movemap_iter,movemap_iter> range;

	for (loc_iter l_it = adj_tiles.begin(); l_it != adj_tiles.end(); ++l_it) {
		range m_its = dstsrc.equal_range(*l_it);
		for(movemap_iter m_it = m_its.first; m_it != m_its.second; ++m_it) {
			const map_location& unit_loc = m_it->second;
			const unit_map::const_iterator u = units.find(unit_loc);
			if (u != units.end()) {
				unit_id[unit_loc] = 0;
			}
			else {
				unit_id[unit_loc] = -1;
			}
		}
	}

	// Start indexing unit loctions.
	typedef std::map<map_location, int>::iterator map_iter;
	int uid = 0;
	for (map_iter m_it = unit_id.begin(); m_it != unit_id.end(); ++m_it) {
		if (m_it->second != -1) {
			m_it->second = uid++;
		}
	}

	// Prepare weight input for max weight matching algorithm.
	max_assignment::weight_matrix weight_mat;
	int size = adj_tiles.size() > uid ? adj_tiles.size() : uid;
	for (int i = 0; i < size; ++i) {
		weight_mat.push_back(std::vector<int>(size, 0));
	}

	// Compute and assign weight values
	int lid = 0;
	for (loc_iter l_it = adj_tiles.begin(); l_it != adj_tiles.end(); ++l_it) {
		range m_its = dstsrc.equal_range(*l_it);
		for(movemap_iter m_it = m_its.first; m_it != m_its.second; ++m_it) {
			const map_location& unit_loc = m_it->second;
			int uid = unit_id[unit_loc];
			if (uid != -1) {
				const unit_map::const_iterator u = units.find(unit_loc);
				const unit& un = *u;
				assert(u != units.end());
				weight_mat[lid][uid] = rate_unit_on_tile(un, *l_it);
			}
		}
		++lid;
	}
	
	max_assignment ma(weight_mat);

	return ma.get_weight_sum() / 100000.;
}

int multi_power_projection_strategy::rate_unit_on_tile(const unit& un, const map_location& tile) const {
	gamemap& map = *resources::game_map;
	
	const t_translation::t_terrain terrain = map[tile];
	
	// The unit might play on the next turn
	int attack_turn = resources::tod_manager->turn();
	if(un.side() < aspect_attacks_ptr_->get_side()) {
		++attack_turn;
	}
	
	// Considering the unit location would be too slow, we only apply the bonus granted by the global ToD
	const int lawful_bonus = resources::tod_manager->get_time_of_day(attack_turn).lawful_bonus;
	int tod_modifier = 0;
	if(un.alignment() == unit_type::LAWFUL) {
		tod_modifier = lawful_bonus;
	}
	else if(un.alignment() == unit_type::CHAOTIC) {
		tod_modifier = -lawful_bonus;
	}
	else if(un.alignment() == unit_type::LIMINAL) {
		tod_modifier = -(abs(lawful_bonus));
	}
	
	// The 0.5 power avoids underestimating too much the damage of a wounded unit.
	int hp = int(sqrt(double(un.hitpoints()) / un.max_hitpoints()) * 1000);
	int most_damage = 0;
	BOOST_FOREACH(const attack_type &att, un.attacks()) {
		int damage = att.damage() * att.num_attacks() * (100 + tod_modifier);
		if (damage > most_damage) {
			most_damage = damage;
		}
	}
	
	int village_bonus = map.is_village(terrain) ? 3 : 2;
	int defense = 100 - un.defense_modifier(terrain);
	int rating = hp * defense * most_damage * village_bonus / 200;
	return rating;
}

} // end of namespace testing_ai_default
	
} // end of namespace ai