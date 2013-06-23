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
 * definitions for analysis strategy base class
 */

#ifndef AI_TESTING_ANALYSIS_STRATEGY_HPP
#define AI_TESTING_ANALYSIS_STRATEGY_HPP

#include <string>
#include "boost/shared_ptr.hpp"

namespace ai {
	
namespace testing_ai_default {
	
// analysis strategy component for default strategy pointer operations
template<class T>
class analysis_strategy_component{
public:
	typedef boost::shared_ptr<T> strategy_ptr;
	const strategy_ptr& get_strategy() const {
		if(concrete_strategy_ptr_ == NULL){
			concrete_strategy_ptr_ = get_default_strategy();
		}
		return concrete_strategy_ptr_;
	}
	
	virtual strategy_ptr get_default_strategy() const = 0;
	virtual void set_strategy_error_handling(){}
	virtual void set_current_strategy(const strategy_ptr& stra_ptr){
		if(stra_ptr != NULL) {
			concrete_strategy_ptr_ = stra_ptr;
		} else {
			set_strategy_error_handling();
		}
	}
	virtual const strategy_ptr& get_current_strategy() const {
		return concrete_strategy_ptr_;
	}
	
	virtual ~analysis_strategy_component(){}
	
private:
	mutable strategy_ptr concrete_strategy_ptr_;
};
	
// analysis strategy base class definitions
class analysis_strategy {
public:
	analysis_strategy() : analysis_strategy_id_("undefined"){}
	virtual ~analysis_strategy(){};

	inline std::string get_id() const {return analysis_strategy_id_;}
	inline void set_id(const std::string& id){analysis_strategy_id_ = id;}
	
protected:	
	std::string analysis_strategy_id_;
};
	
} // end of namespace testing_ai_default
	
} // end of namespace ai

#endif
