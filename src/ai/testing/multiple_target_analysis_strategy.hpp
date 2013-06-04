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

#ifndef AI_TESTING_MULTIPLE_TARGET_ANALYSIS_STRATEGY_HPP
#define AI_TESTING_MULTIPLE_TARGET_ANALYSIS_STRATEGY_HPP

#include "target_analysis_strategy.hpp"

namespace ai {
    
namespace testing_ai_default {
        
class multiple_target_analysis_strategy : public target_analysis_strategy1 {
public:
    struct analysis_inputs {
        const move_map& srcdst; 
        const move_map& dstsrc;
        const move_map& fullmove_srcdst;
        const move_map& fullmove_dstsrc;
        const move_map& enemy_srcdst; 
        const move_map& enemy_dstsrc;
        const team& current_team;
        
        analysis_inputs (const move_map& srcdst_val,
                         const move_map& dstsrc_val,
                         const move_map& fullmove_srcdst_val,
                         const move_map& fullmove_dstsrc_val,
                         const move_map& enemy_srcdst_val,
                         const move_map& enemy_dstsrc_val,
                         const team& current_team_val) 
        : srcdst(srcdst_val), 
          dstsrc(dstsrc_val),
          fullmove_srcdst(fullmove_srcdst_val),
          fullmove_dstsrc(fullmove_dstsrc_val),
          enemy_srcdst(enemy_srcdst_val),
          enemy_dstsrc(enemy_dstsrc_val),
          current_team(current_team_val){
            
        }
          
    };
    
    typedef std::vector< std::vector<map_location> > adj_tiles_vec;
    typedef std::map<map_location, bool> used_location_map;
    
    multiple_target_analysis_strategy(int target_number=2);
    
    virtual boost::shared_ptr<attacks_vector> analyze_targets_impl(const aspect_attacks& asp_atks) const;
    
    virtual void do_multiple_target_analysis(const analysis_inputs& ana_inp, 
                                             const std::vector<map_location>& target_locs, 
                                             const int target_count, 
                                             const int target_number, 
                                             const aspect_attacks& asp_atks,
                                             std::vector<map_location>& units,
                                             std::vector<map_location>& cur_target_locs, 
                                             std::vector<attack_analysis>& result) const;
    
    virtual void do_attack_analysis(const analysis_inputs& ana_inp,
                                    const std::vector<map_location>& target_locs,
                                    const adj_tiles_vec& adj_tiles,
                                    const readonly_context *ai_obj,
                                    std::vector<map_location>& units,
                                    used_location_map& used_locations,
                                    attack_analysis& cur_analysis,
                                    std::vector<attack_analysis>& result) const;
private:
    int target_number_;
    static const int MAX_TARGET_NUMBER = 3;
};
        
} // end of namespace testing_ai_default
    
} // end of namespace ai

#endif
