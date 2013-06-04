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
#include "../../global.hpp"

#include "ai.hpp"
#include "../actions.hpp"
#include "../manager.hpp"

#include "../../attack_prediction.hpp"
#include "../../game_config.hpp"
#include "../../log.hpp"
#include "../../map.hpp"
#include "../../team.hpp"

#include "attack_analysis_strategy.hpp"

static lg::log_domain log_ai_testing_multi_target("ai/multi_target");
#define DBG_AI_MT LOG_STREAM(debug, log_ai_testing_multi_target)
#define LOG_AI_MT LOG_STREAM(info, log_ai_testing_multi_target)
#define ERR_AI_MT LOG_STREAM(err, log_ai_testing_multi_target)

static lg::log_domain log_ai("ai/attack");
#define LOG_AI LOG_STREAM(info, log_ai)
#define ERR_AI LOG_STREAM(err, log_ai)

namespace ai {
    
namespace testing_ai_default {

void attack_analysis_strategy1::analyze_impl(attack_analysis& aas,
                                             const gamemap& map, 
                                             unit_map& units,
                                             const readonly_context& ai_obj,
                                             const move_map& dstsrc, 
                                             const move_map& srcdst,
                                             const move_map& enemy_dstsrc, 
                                             double aggression) const{
    std::vector<attack_analysis::analysis_result>& ana_res = aas.analysis_results;
    int target_number = ana_res.size();
    std::vector<unit_map::const_iterator> defenders;
    std::vector<double> def_avg_experience;
    std::vector<double> first_chance_kill;
    std::vector<double> prob_dead_already;
    for(size_t i=0;i<target_number;++i){
        unit_map::const_iterator defend_it = units.find(ana_res.at(i).target);
        assert(defend_it != units.end());
        defenders.push_back(defend_it);
    
        // See if the target is a threat to our leader or an ally's leader.
        map_location adj[6];
        get_adjacent_tiles(ana_res.at(i).target,adj);
        size_t tile;
        for(tile = 0; tile != 6; ++tile) {
            const unit_map::const_iterator leader = units.find(adj[tile]);
            if(leader != units.end() && leader->can_recruit() && !ai_obj.current_team().is_enemy(leader->side())) {
                break;
            }
        }
        
        ana_res.at(i).leader_threat = (tile != 6);
        ana_res.at(i).uses_leader = false;
        
        ana_res.at(i).target_value = defend_it->cost();
        ana_res.at(i).target_value += (double(defend_it->experience())/
                         double(defend_it->max_experience()))*ana_res.at(i).target_value;
        ana_res.at(i).target_starting_damage = defend_it->max_hitpoints() - defend_it->hitpoints();
        
        // Calculate the 'alternative_terrain_quality' -- the best possible defensive values
        // the attacking units could hope to achieve if they didn't attack and moved somewhere.
        // This is used for comparative purposes, to see just how vulnerable the AI is
        // making itself.
        ana_res.at(i).alternative_terrain_quality = 0.0;
        double cost_sum = 0.0;
        for(size_t j = 0; j != aas.movements.size(); ++j) {
            const unit_map::const_iterator att = units.find(aas.movements.at(j).first);
            const double cost = att->cost();
            cost_sum += cost;
            ana_res.at(i).alternative_terrain_quality += cost*ai_obj.best_defensive_position(aas.movements.at(j).first,dstsrc,srcdst,enemy_dstsrc).chance_to_hit;
        }
        ana_res.at(i).alternative_terrain_quality /= cost_sum*100;
        
        ana_res.at(i).avg_damage_inflicted = 0.0;
        ana_res.at(i).avg_damage_taken = 0.0;
        ana_res.at(i).resources_used = 0.0;
        ana_res.at(i).terrain_quality = 0.0;
        ana_res.at(i).avg_losses = 0.0;
        ana_res.at(i).chance_to_kill = 0.0;
        ana_res.at(i).is_visited = false;
        
        def_avg_experience.push_back(0.0);
        first_chance_kill.push_back(0.0);
        
        prob_dead_already.push_back(0.0);
    }

    assert(!aas.movements.empty());
    
    //std::vector<std::pair<map_location,map_location> >::const_iterator m;
    
    typedef boost::shared_ptr<battle_context> battle_context_ptr;
    boost::shared_array<battle_context_ptr> prev_bc(new battle_context_ptr[target_number]);
    boost::shared_array<const combatant *> prev_def(new const combatant *[target_number]);
    for(size_t i=0;i<target_number;++i){
        prev_def[i] = NULL;
    }
    
	for (size_t i=0; i<aas.movements.size(); ++i) {
        // Log this movement
        LOG_AI_MT<<"The movement: from " <<aas.movements.at(i).first << " to " << aas.movements.at(i).second << std::endl;
        
        // Target id for this movement
        // single target use target id 0 by default
        int tid = aas.movements_target_ids.empty() ? 0 : aas.movements_target_ids.at(i);

        ana_res.at(tid).is_visited = true;
        const std::pair<map_location,map_location>& m = aas.movements.at(i);
		// We fix up units map to reflect what this would look like.
		unit *up = units.extract(m.first);
		up->set_location(m.second);
		units.insert(up);
		double m_aggression = aggression;
        
		if (up->can_recruit()) {
			ana_res.at(tid).uses_leader = true;
			// FIXME: suokko's r29531 omitted this line
			ana_res.at(tid).leader_threat = false;
			m_aggression = ai_obj.get_leader_aggression();
		}
        
		bool from_cache = false;
        boost::shared_array<battle_context_ptr> bc(new battle_context_ptr[target_number]);
        
		// This cache is only about 99% correct, but speeds up evaluation by about 1000 times.
		// We recalculate when we actually attack.
		std::map<std::pair<map_location, const unit_type *>,std::pair<battle_context_unit_stats,battle_context_unit_stats> >::iterator usc;
        
		const unit_type *up_type = up->type();
		if(up_type) {
			usc = ai_obj.unit_stats_cache().find(std::pair<map_location, const unit_type *>(ana_res.at(tid).target, up_type));
		} else {
			usc = ai_obj.unit_stats_cache().end();
		}
		// Just check this attack is valid for this attacking unit (may be modified)
		if (usc != ai_obj.unit_stats_cache().end() &&
            usc->second.first.attack_num <
            static_cast<int>(up->attacks().size())) {
            
			from_cache = true;
			bc[tid].reset(new battle_context(usc->second.first, usc->second.second));
            LOG_AI_MT << "Old bc found!" << std::endl;
		} else {
			bc[tid].reset(new battle_context(units, m.second, ana_res.at(tid).target, -1, -1, m_aggression, prev_def[tid]));
            LOG_AI_MT << "New bc created!" << std::endl;
		}
		const combatant &att = bc[tid]->get_attacker_combatant(prev_def[tid]);
		const combatant &def = bc[tid]->get_defender_combatant(prev_def[tid]);
        
		prev_def[tid] = &bc[tid]->get_defender_combatant(prev_def[tid]);
        prev_bc[tid] = bc[tid];
        
		if (!from_cache && up_type) {
			ai_obj.unit_stats_cache().insert(std::pair<std::pair<map_location, const unit_type *>,std::pair<battle_context_unit_stats,battle_context_unit_stats> >
                                             (std::pair<map_location, const unit_type *>(ana_res.at(tid).target, up_type),
                                              std::pair<battle_context_unit_stats,battle_context_unit_stats>(bc[tid]->get_attacker_stats(),
                                                                                                             bc[tid]->get_defender_stats())));
		}
        
		// Note we didn't fight at all if defender is already dead.
		double prob_fought = (1.0 - prob_dead_already.at(tid));
        
		/** @todo 1.9 add combatant.prob_killed */
		double prob_killed = def.hp_dist[0] - prob_dead_already.at(tid);
		prob_dead_already.at(tid) = def.hp_dist[0];
        
		double prob_died = att.hp_dist[0];
		double prob_survived = (1.0 - prob_died) * prob_fought;
        
		double cost = up->cost();
		const bool on_village = map.is_village(m.second);
		// Up to double the value of a unit based on experience
		cost += (double(up->experience()) / up->max_experience())*cost;
		ana_res.at(tid).resources_used += cost;
		ana_res.at(tid).avg_losses += cost * prob_died;
        
		// add half of cost for poisoned unit so it might get chance to heal
		ana_res.at(tid).avg_losses += cost * up->get_state(unit::STATE_POISONED) /2;
        
		// Double reward to emphasize getting onto villages if they survive.
		if (on_village) {
			ana_res.at(tid).avg_damage_taken -= game_config::poison_amount*2 * prob_survived;
		}
        
		ana_res.at(tid).terrain_quality += (double(bc[tid]->get_defender_stats().chance_to_hit)/100.0)*cost * (on_village ? 0.5 : 1.0);
        
		double advance_prob = 0.0;
		// The reward for advancing a unit is to get a 'negative' loss of that unit
		if (!up->advances_to().empty()) {
			int xp_for_advance = up->max_experience() - up->experience();
            
			// See bug #6272... in some cases, unit already has got enough xp to advance,
			// but hasn't (bug elsewhere?).  Can cause divide by zero.
			if (xp_for_advance <= 0)
				xp_for_advance = 1;
            
			int fight_xp = defenders.at(tid)->level();
			int kill_xp = game_config::kill_xp(fight_xp);
            
			if (fight_xp >= xp_for_advance) {
				advance_prob = prob_fought;
				ana_res.at(tid).avg_losses -= up->cost() * prob_fought;
			} else if (kill_xp >= xp_for_advance) {
				advance_prob = prob_killed;
				ana_res.at(tid).avg_losses -= up->cost() * prob_killed;
				// The reward for getting a unit closer to advancement
				// (if it didn't advance) is to get the proportion of
				// remaining experience needed, and multiply it by
				// a quarter of the unit cost.
				// This will cause the AI to heavily favor
				// getting xp for close-to-advance units.
				ana_res.at(tid).avg_losses -= up->cost() * 0.25 *
                fight_xp * (prob_fought - prob_killed)
                / xp_for_advance;
			} else {
				ana_res.at(tid).avg_losses -= up->cost() * 0.25 *
                (kill_xp * prob_killed + fight_xp * (prob_fought - prob_killed))
                / xp_for_advance;
			}
            
			// The reward for killing with a unit that plagues
			// is to get a 'negative' loss of that unit.
			if (bc[tid]->get_attacker_stats().plagues) {
				ana_res.at(tid).avg_losses -= prob_killed * up->cost();
			}
		}
        
		// If we didn't advance, we took this damage.
		ana_res.at(tid).avg_damage_taken += (up->hitpoints() - att.average_hp()) * (1.0 - advance_prob);
        
		/**
		 * @todo 1.9: attack_prediction.cpp should understand advancement
		 * directly.  For each level of attacker def gets 1 xp or
		 * kill_experience.
		 */
		int fight_xp = up->level();
		int kill_xp = game_config::kill_xp(fight_xp);
		def_avg_experience.at(tid) += fight_xp * (1.0 - att.hp_dist[0]) + kill_xp * att.hp_dist[0];
		if (i == 0) { // first movement
			first_chance_kill.at(tid) = def.hp_dist[0];
		}
	}
    
    for(size_t i=0;i<target_number;++i){
        if(!ana_res.at(i).is_visited) {
            // If not a target in this attack. 
            // do not compute values
            continue;
        }
        
        if (!defenders.at(i)->advances_to().empty() &&
            def_avg_experience.at(i) >= defenders.at(i)->max_experience() - defenders.at(i)->experience()) {
            // It's likely to advance: only if we can kill with first blow.
            ana_res.at(i).chance_to_kill = first_chance_kill.at(i);
            // Negative average damage (it will advance).
            ana_res.at(i).avg_damage_inflicted = defenders.at(i)->hitpoints() - defenders.at(i)->max_hitpoints();
        } else {
            ana_res.at(i).chance_to_kill = prev_def[i]->hp_dist[0];
            ana_res.at(i).avg_damage_inflicted = defenders.at(i)->hitpoints() - prev_def[i]->average_hp(map.gives_healing(defenders.at(i)->get_location()));
        }
        ana_res.at(i).terrain_quality /= ana_res.at(i).resources_used;
    }
    
	// Restore the units to their original positions.
	for (size_t i=0; i<aas.movements.size(); ++i) {
        const std::pair<map_location,map_location>& m = aas.movements.at(i);
		units.move(m.second, m.first);
	}
    
    // Set default values to be the values corresponding
    // to the target of first movement
    int tid = aas.movements_target_ids.empty() ? 0 : aas.movements_target_ids.at(0);
    
    aas.target = ana_res.at(tid).target;
    aas.target_value = ana_res.at(tid).target_value;
    aas.avg_losses = ana_res.at(tid).avg_losses;
    aas.chance_to_kill = ana_res.at(tid).chance_to_kill;
    aas.avg_damage_inflicted = ana_res.at(tid).avg_damage_inflicted;
    aas.target_starting_damage = ana_res.at(tid).target_starting_damage;
    aas.avg_damage_taken = ana_res.at(tid).avg_damage_taken;
    aas.resources_used = ana_res.at(tid).resources_used;
    aas.terrain_quality = ana_res.at(tid).terrain_quality;
    aas.alternative_terrain_quality = ana_res.at(tid).alternative_terrain_quality;
    aas.vulnerability = ana_res.at(tid).vulnerability;
    aas.support = ana_res.at(tid).support;
    aas.leader_threat = ana_res.at(tid).leader_threat;
    aas.uses_leader = ana_res.at(tid).uses_leader;
    aas.is_surrounded = ana_res.at(tid).is_surrounded;
}

double attack_analysis_strategy1::rating_impl(const attack_analysis& aas, double aggression, const readonly_context& ai_obj) const{
    
    const std::vector<attack_analysis::analysis_result>& ana_res = aas.analysis_results;
    int target_number = ana_res.size();
    
    // FIXME: attack with more targets get higher rating? (not expected if it's the case)
    
    double result = 0.0;
    for (size_t i=0; i<target_number; ++i) {
        
        if(!ana_res.at(i).is_visited || ana_res.at(i).movements.empty()) {
            continue;
        }
    
        if(ana_res.at(i).leader_threat) {
            aggression = 1.0;
        }
        
        if(ana_res.at(i).uses_leader) {
            aggression = ai_obj.get_leader_aggression();
        }
        
        double value = ana_res.at(i).chance_to_kill*ana_res.at(i).target_value - ana_res.at(i).avg_losses*(1.0-aggression);
        
        if(ana_res.at(i).terrain_quality > ana_res.at(i).alternative_terrain_quality) {
            // This situation looks like it might be a bad move:
            // we are moving our attackers out of their optimal terrain
            // into sub-optimal terrain.
            // Calculate the 'exposure' of our units to risk.
            
    #ifdef SUOKKO
            //FIXME: this code was in sukko's r29531  Correct?
            const double exposure_mod = uses_leader ? ai_obj.current_team().caution()* 8.0 : ai_obj.current_team().caution() * 4.0;
            const double exposure = exposure_mod*resources_used*((terrain_quality - alternative_terrain_quality)/10)*vulnerability/std::max<double>(0.01,support);
    #else
            const double exposure_mod = ana_res.at(i).uses_leader ? 2.0 : ai_obj.get_caution();
            const double exposure = exposure_mod*ana_res.at(i).resources_used*(ana_res.at(i).terrain_quality - ana_res.at(i).alternative_terrain_quality)*ana_res.at(i).vulnerability/std::max<double>(0.01,ana_res.at(i).support);
    #endif
            LOG_AI << "attack option has base value " << value << " with exposure " << exposure << ": "
            << ana_res.at(i).vulnerability << "/" << ana_res.at(i).support << " = " << (ana_res.at(i).vulnerability/std::max<double>(ana_res.at(i).support,0.1)) << "\n";
            value -= exposure*(1.0-aggression);
        }
        
        // Prefer to attack already damaged targets.
        value += ((ana_res.at(i).target_starting_damage/3 + ana_res.at(i).avg_damage_inflicted) - (1.0-aggression)*ana_res.at(i).avg_damage_taken)/10.0;
        
        // If the unit is surrounded and there is no support,
        // or if the unit is surrounded and the average damage is 0,
        // the unit skips its sanity check and tries to break free as good as possible.
        if(!ana_res.at(i).is_surrounded || (ana_res.at(i).support != 0 && ana_res.at(i).avg_damage_taken != 0))
        {
            // Sanity check: if we're putting ourselves at major risk,
            // and have no chance to kill, and we're not aiding our allies
            // who are also attacking, then don't do it.
            if(ana_res.at(i).vulnerability > 50.0 && ana_res.at(i).vulnerability > ana_res.at(i).support*2.0
               && ana_res.at(i).chance_to_kill < 0.02 && aggression < 0.75
               && !aas.attack_close(ana_res.at(i).target)) {
                return -1.0;
            }
        }
        
        if(!ana_res.at(i).leader_threat && ana_res.at(i).vulnerability*ana_res.at(i).terrain_quality > 0.0) {
            value *= ana_res.at(i).support/(ana_res.at(i).vulnerability*ana_res.at(i).terrain_quality);
        }
        
        value /= ((ana_res.at(i).resources_used/2) + (ana_res.at(i).resources_used/2)*ana_res.at(i).terrain_quality);
        
        if(ana_res.at(i).leader_threat) {
            value *= 5.0;
        }
        
        result += value;
        
        LOG_AI << "attack on " << ana_res.at(i).target << ": attackers: " << ana_res.at(i).movements.size()
        << " value: " << value << " chance to kill: " << ana_res.at(i).chance_to_kill
        << " damage inflicted: " << ana_res.at(i).avg_damage_inflicted
        << " damage taken: " << ana_res.at(i).avg_damage_taken
        << " vulnerability: " << ana_res.at(i).vulnerability
        << " support: " << ana_res.at(i).support
        << " quality: " << ana_res.at(i).terrain_quality
        << " alternative quality: " << ana_res.at(i).alternative_terrain_quality << "\n";
    }
    
    return result;
}
    
} // end of namespace testing_ai_default
    
} // end of namespace ai