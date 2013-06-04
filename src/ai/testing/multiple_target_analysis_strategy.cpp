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
 * analysis_strategy: multiple target
 */

#include "../manager.hpp"
#include "../../actions.hpp"
#include "../../log.hpp"
#include "../../map.hpp"
#include "../../team.hpp"
#include "../../tod_manager.hpp"
#include "../../resources.hpp"
#include "../../unit.hpp"
#include "../../pathfind/pathfind.hpp"
#include "aspect_attacks.hpp"
#include "multiple_target_analysis_strategy.hpp"

static lg::log_domain log_ai_testing_multi_target("ai/multi_target");
#define DBG_AI LOG_STREAM(debug, log_ai_testing_multi_target)
#define LOG_AI LOG_STREAM(info, log_ai_testing_multi_target)
#define ERR_AI LOG_STREAM(err, log_ai_testing_multi_target)

namespace ai {
    
namespace testing_ai_default {

multiple_target_analysis_strategy::multiple_target_analysis_strategy(int target_number)
    : target_number_(target_number){
    if(target_number > MAX_TARGET_NUMBER){
        LOG_AI<<"multiple target analysis can currently support not more than" << MAX_TARGET_NUMBER << "targets." << std::endl;
        target_number_ = MAX_TARGET_NUMBER;
    } else if(target_number <= 0){
        ERR_AI<< "the target number is invalid" << std::endl;
        target_number_ = MAX_TARGET_NUMBER;
    }
    set_id("multiple_target_analysis_strategy");
}
    
boost::shared_ptr<attacks_vector> multiple_target_analysis_strategy::analyze_targets_impl(const aspect_attacks& asp_atks) const{
    const move_map& srcdst = asp_atks.get_srcdst();
    const move_map& dstsrc = asp_atks.get_dstsrc();
    const move_map& enemy_srcdst = asp_atks.get_enemy_srcdst();
    const move_map& enemy_dstsrc = asp_atks.get_enemy_dstsrc();
    
    boost::shared_ptr<attacks_vector> res(new attacks_vector());
    unit_map& units_ = *resources::units;
    
    std::vector<map_location> unit_locs;
    for(unit_map::const_iterator i = units_.begin(); i != units_.end(); ++i) {
        if (i->side() == asp_atks.get_side() && i->attacks_left() && !(i->can_recruit() && asp_atks.get_passive_leader())) {
            if (!i->matches_filter(vconfig(asp_atks.get_filter_own()), i->get_location())) {
                continue;
            }
            unit_locs.push_back(i->get_location());
        }
    }
    
    moves_map dummy_moves;
    move_map fullmove_srcdst, fullmove_dstsrc;
    asp_atks.calculate_possible_moves(dummy_moves,fullmove_srcdst,fullmove_dstsrc,false,true);
    
    asp_atks.unit_stats_cache().clear();
    
    std::vector<map_location> target_locs;
    for(unit_map::const_iterator i = units_.begin(); i != units_.end(); ++i) {
        // Attack anyone who is on the enemy side,
		// and who is not invisible or petrified.
        // save enemies in a vector
		if (asp_atks.current_team().is_enemy(i->side()) && !i->incapacitated() &&
		    !i->invisible(i->get_location()))
		{
			if (!i->matches_filter(vconfig(asp_atks.get_filter_enemy()), i->get_location())) {
				continue;
			}
            target_locs.push_back(i->get_location());
		}
	}
    
    int target_number = target_number_;
    // there may not be enough number of targets
    if(target_locs.size() < target_number){
        target_number = target_locs.size();
    }
    
    analysis_inputs ana_inp(srcdst, dstsrc, fullmove_srcdst, fullmove_dstsrc, enemy_srcdst, enemy_dstsrc, asp_atks.current_team()); 
    
    if(target_number != 0){
        std::vector<map_location> cur_target_locs;
        cur_target_locs.reserve(target_number);
        do_multiple_target_analysis(ana_inp, target_locs, 0, target_number, asp_atks, unit_locs, cur_target_locs, *res);
    }
    
	return res;
}
    
void multiple_target_analysis_strategy::do_multiple_target_analysis(const analysis_inputs& ana_inp, 
                                                                    const std::vector<map_location>& target_locs, 
                                                                    const int target_count, 
                                                                    const int target_number, 
                                                                    const aspect_attacks& asp_atks,
                                                                    std::vector<map_location>& units,
                                                                    std::vector<map_location>& cur_target_locs,
                                                                    std::vector<attack_analysis>& result) const{
    // Generate target permutation
    // whenever we have required number of target
    // do attack analysis with these targets
    if(target_count == target_number){
        // Create the attack_analysis instance based on 
        // the analysis strategy from config in aspect_attack
        const config& analysis_strategy_cfg = asp_atks.get_analysis_strategy_cfg();
        attack_analysis analysis(analysis_strategy_cfg, target_number);
        
        //analysis.target = cur_target_locs[0];
        for(int i=0;i<cur_target_locs.size();++i){
            analysis.analysis_results.at(i).target = cur_target_locs.at(i);
        }
        
        // Compute adjacent tiles of the targets
        adj_tiles_vec adj_tiles;
        adj_tiles.reserve(target_number);
        map_location adj_array[6];
        for(size_t i=0;i<cur_target_locs.size();++i){
            //boost::shared_array<map_location> adj(new map_location[6]);
			get_adjacent_tiles(cur_target_locs.at(i), adj_array);
            std::vector<map_location> adj_vec(adj_array, adj_array+6);
            adj_tiles.push_back(adj_vec);
        }
        
        // Construct used locations map.
        // the map is used because tiles of 
        // targets can overlap with each other
        used_location_map used_locations;
        for(size_t i=0;i<adj_tiles.size();++i){
            for(size_t j=0;j<6;++j){
                used_locations[adj_tiles.at(i).at(j)] = false;
            }
        }
        
        do_attack_analysis(ana_inp, cur_target_locs, adj_tiles, &asp_atks, units, used_locations, analysis, result);
        
        return;
    }
    
    for(size_t i=target_count;i<=target_locs.size()-(target_number-target_count);++i){
        cur_target_locs.push_back(target_locs.at(i));
        do_multiple_target_analysis(ana_inp, target_locs, target_count+1, target_number, asp_atks, units, cur_target_locs, result);
        cur_target_locs.pop_back();
    }
}
    
void multiple_target_analysis_strategy::do_attack_analysis(const analysis_inputs& ana_inp,
                                                           const std::vector<map_location>& target_locs,
                                                           const adj_tiles_vec& adj_tiles,
                                                           const readonly_context *ai_obj,
                                                           std::vector<map_location>& units,
                                                           used_location_map& used_locations,
                                                           attack_analysis& cur_analysis,
                                                           std::vector<attack_analysis>& result) const{
    // This function is called fairly frequently, so interact with the user here.
    
    ai::manager::raise_user_interact();
    const int default_attack_depth = 5;
    if(cur_analysis.movements.size() >= size_t(default_attack_depth)) {
        //std::cerr << "ANALYSIS " << cur_analysis.movements.size() << " >= " << get_attack_depth() << "\n";
        return;
    }
    gamemap &map_ = *resources::game_map;
    unit_map &units_ = *resources::units;
    std::vector<team> &teams_ = *resources::teams;
    
    
    const size_t max_positions = 1000;
    if(result.size() > max_positions && !cur_analysis.movements.empty()) {
        LOG_AI << "cut analysis short with number of positions\n";
        return;
    }
    
    for(size_t i = 0; i != units.size(); ++i) {
        const map_location current_unit = units[i];
        
        unit_map::iterator unit_itor = units_.find(current_unit);
        assert(unit_itor != units_.end());
        
        // See if the unit has the backstab ability.
        // Units with backstab will want to try to have a
        // friendly unit opposite the position they move to.
        //
        // See if the unit has the slow ability -- units with slow only attack first.
        bool backstab = false, slow = false;
        std::vector<attack_type>& attacks = unit_itor->attacks();
        for(std::vector<attack_type>::iterator a = attacks.begin(); a != attacks.end(); ++a) {
            a->set_specials_context(map_location(), map_location(), units_, true, NULL);
            if(a->get_special_bool("backstab")) {
                backstab = true;
            }
            
            if(a->get_special_bool("slow")) {
                slow = true;
            }
        }
        
        if(slow && cur_analysis.movements.empty() == false) {
            continue;
        }
        
        // Check if the friendly unit is surrounded,
        // A unit is surrounded if it is flanked by enemy units
        // and at least one other enemy unit is nearby
        // or if the unit is totaly surrounded by enemies
        // with max. one tile to escape.
        bool is_surrounded = false;
        bool is_flanked = false;
        int enemy_units_around = 0;
        int accessible_tiles = 0;
        map_location adj[6];
        get_adjacent_tiles(current_unit, adj);
        
        size_t tile;
        for(tile = 0; tile != 3; ++tile) {
            
            const unit_map::const_iterator tmp_unit = units_.find(adj[tile]);
            bool possible_flanked = false;
            
            if(map_.on_board(adj[tile]))
            {
                accessible_tiles++;
                if (tmp_unit != units_.end() && ana_inp.current_team.is_enemy(tmp_unit->side()))
                {
                    enemy_units_around++;
                    possible_flanked = true;
                }
            }
            
            const unit_map::const_iterator tmp_opposite_unit = units_.find(adj[tile + 3]);
            if(map_.on_board(adj[tile + 3]))
            {
                accessible_tiles++;
                if (tmp_opposite_unit != units_.end() && ana_inp.current_team.is_enemy(tmp_opposite_unit->side()))
                {
                    enemy_units_around++;
                    if(possible_flanked)
                    {
                        is_flanked = true;
                    }
                }
            }
        }
        
        if((is_flanked && enemy_units_around > 2) || enemy_units_around >= accessible_tiles - 1)
            is_surrounded = true;
        
        
        
        double best_vulnerability = 0.0, best_support = 0.0;
        int best_rating = 0;
        int cur_position = -1;
        int best_target_id = 0;
        
        // Iterate over positions adjacent to all the targets, finding the best rated one.
        for(size_t j = 0; j < adj_tiles.size(); ++j) {
            const std::vector<map_location>& adj_locs = adj_tiles.at(j);
            const map_location& target = target_locs.at(j);
            
            for(size_t k = 0;k < 6; ++k){
                const map_location& loc = adj_locs.at(k);
                // If in this planned attack, a unit is already in this location.
                if(used_locations[loc]) {
                    continue;
                }
                
                // See if the current unit can reach that position.
                if (loc != current_unit) {
                    typedef std::multimap<map_location,map_location>::const_iterator Itor;
                    std::pair<Itor,Itor> its = ana_inp.dstsrc.equal_range(loc);
                    while(its.first != its.second) {
                        if(its.first->second == current_unit)
                            break;
                        ++its.first;
                    }
                    
                    // If the unit can't move to this location.
                    if(its.first == its.second || units_.find(loc) != units_.end()) {
                        continue;
                    }
                }
                
                unit_ability_list abil = unit_itor->get_abilities("leadership",loc);
                int best_leadership_bonus = abil.highest("value").first;
                double leadership_bonus = static_cast<double>(best_leadership_bonus+100)/100.0;
                if (leadership_bonus > 1.1) {
                    LOG_AI << unit_itor->name() << " is getting leadership " << leadership_bonus << "\n";
                }
                
                // Check to see whether this move would be a backstab.
                bool is_backstab = false, is_surround = false; // is_surround: to surround target, different from is_surrounded defined earlier which means surrounded by enemy
                
                if(adj_locs[(k+3)%6] != current_unit) {
                    const unit_map::const_iterator itor = units_.find(adj_locs[(k+3)%6]);
                    
                    // Note that we *could* also check if a unit plans to move there
                    // before we're at this stage, but we don't because, since the
                    // attack calculations don't actually take backstab into account (too complicated),
                    // this could actually make our analysis look *worse* instead of better.
                    // So we only check for 'concrete' backstab opportunities.
                    // That would also break backstab_check, since it assumes
                    // the defender is in place.
                    if(itor != units_.end() &&
                       backstab_check(loc, target, units_, teams_)) {
                        if(backstab) {
                            is_backstab = true;
                        }
                        
                        // No surround bonus if target is skirmisher
                        if (!itor->get_ability_bool("skirmisher")){
                            is_surround = true;
                        }
                    }
                    
                    
                }
                
                // See if this position is the best rated we've seen so far.
                int rating = static_cast<int>(rate_terrain(*unit_itor, loc, is_backstab));
                if(cur_position >= 0 && rating < best_rating) {
                    continue;
                }
                
                // Find out how vulnerable we are to attack from enemy units in this hex.
                // and how much support we have on this hex from allies.
                //FIXME: suokko's r29531 multiplied this by a constant 1.5. ?
                
                double vulnerability = 0.;
                double support = 0.;
                rate_vulnerability_support(loc, ana_inp.enemy_dstsrc, ana_inp.fullmove_dstsrc, is_surround, vulnerability, support);
                
                // If this is a position with equal defense to another position,
                // but more vulnerability then we don't want to use it.
    #ifdef SUOKKO
                //FIXME: this code was in sukko's r29531  Correct?
                // scale vulnerability to 60 hp unit
                if(cur_position >= 0 && rating < best_rating
                   && (vulnerability*30.0)/unit_itor->second.hitpoints() -
                   (support*30.0)/unit_itor->second.max_hitpoints()
                   > best_vulnerability - best_support) {
                    continue;
                }
    #else
                if(cur_position >= 0 && rating == best_rating && vulnerability - support >= best_vulnerability - best_support) {
                    continue;
                }
    #endif
                cur_position = k;
                best_rating = rating;
                best_target_id = j;
    #ifdef SUOKKO
                //FIXME: this code was in sukko's r29531  Correct?
                best_vulnerability = (vulnerability*30.0)/unit_itor->second.hitpoints();
                best_support = (support*30.0)/unit_itor->second.max_hitpoints();
    #else
                best_vulnerability = vulnerability;
                best_support = support;
    #endif
            }
        }
        
        if(cur_position != -1) {
            const map_location& best_tile = adj_tiles.at(best_target_id).at(cur_position);
            units.erase(units.begin() + i);
            
            cur_analysis.analysis_results.at(best_target_id).movements.push_back(attack_analysis::movement_step(current_unit,best_tile));
            
            cur_analysis.movements.push_back(attack_analysis::movement_step(current_unit,best_tile));
            cur_analysis.movements_target_ids.push_back(best_target_id);
            
            cur_analysis.analysis_results.at(best_target_id).vulnerability += best_vulnerability;
            
            cur_analysis.analysis_results.at(best_target_id).support += best_support;
            
            cur_analysis.analysis_results.at(best_target_id).is_surrounded = is_surrounded;
            
            // log debug information, test only
            LOG_AI<<"Total movements: "<<cur_analysis.movements.size()<<std::endl;
            LOG_AI<<"Target IDs: "<<std::endl;
            for(size_t j=0;j<cur_analysis.movements_target_ids.size();++j){
                LOG_AI<<cur_analysis.movements_target_ids.at(j)<<std::endl;
            }
            
            if (ai_obj!=NULL) {
                cur_analysis.analyze(map_, units_, *ai_obj, ana_inp.dstsrc, ana_inp.srcdst, ana_inp.enemy_dstsrc, ai_obj->get_aggression());
            }
            result.push_back(cur_analysis);
            used_locations[best_tile] = true;
            do_attack_analysis(ana_inp, target_locs, adj_tiles, ai_obj, units, used_locations, cur_analysis, result);
            used_locations[best_tile] = false;
            
            
            cur_analysis.analysis_results.at(best_target_id).vulnerability -= best_vulnerability;
            cur_analysis.analysis_results.at(best_target_id).support -= best_support;
            
            cur_analysis.analysis_results.at(best_target_id).movements.pop_back();
            cur_analysis.movements.pop_back();
            cur_analysis.movements_target_ids.pop_back();
            
            units.insert(units.begin() + i, current_unit);
        }
    }
}
    
} // end of namespace testing_ai_default
    
} // end of namespace ai