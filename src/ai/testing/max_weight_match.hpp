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
 * Max weight hungary match algorithm on
 * complete bipartite graph.
 * Input: square weight matrix
 * Output: max weight match
 */

#ifndef AI_TESTING_MAX_WEIGHT_MATCH_HPP
#define AI_TESTING_MAX_WEIGHT_MATCH_HPP

#include <vector>

namespace ai {

namespace testing_ai_default {

class max_assignment {
public:
	typedef std::vector< std::vector<bool> > match_matrix;
	typedef std::vector< std::vector<int> > weight_matrix;
	max_assignment(const weight_matrix& weight_mat);
	inline const match_matrix& get_match() {
		return match_;
	}

	inline bool get_match(const int i, const int j) {
		return match_[i][j];
	}

	inline int get_size() {
		return size_;
	}
	
	inline double get_weight_sum() {
		return weight_sum_;
	}

	inline int get_left_match(const int right_id) {
		if(right_id < 0 || right_id >= size_) return -1;
		return matched_on_left_[right_id];
	}

	inline int get_right_match(const int left_id) {
		if(left_id < 0 || left_id >= size_) return -1;
		return matched_on_right_[left_id];
	}
private:
	/**
	 * weighted hungary algorithm computing max weight match
	 * of bipatite graph
	 */
	void weighted_hungary_routine();

	/**
	 * Main routing function of hungray algorithm searching for
	 * augmenting path in equality graph.
	 */
	void do_augment(std::vector<bool>& left_ismatched,
					std::vector<bool>& right_ismatched,
					std::vector<int>& prev_left,
					std::vector<int>& weight_label_diff,
					std::vector<int>& weight_label_diff_left_vertex,
					int& matched_size,
					std::vector<int>& left_label,
					std::vector<int>& right_label);

	/**
	 * function to update labels if augmenting path is
	 * not found in BFS
	 */
	void update_labels(const std::vector<bool>& right_ismatched,
					   const std::vector<bool>& left_ismatched,
					   std::vector<int>& weight_label_diff,
					   std::vector<int>& left_label,
					   std::vector<int>& right_label);

	/**
	 * function to add edge to equality graph when a qualified
	 * edge is found.
	 */
	void add_edge_to_equality_graph(const int left,
									const int prev_left_vertex,
									const std::vector<int>& left_label,
									const std::vector<int>& right_label,
									std::vector<bool>& left_ismatched,
									std::vector<int>& prev_left,
									std::vector<int>& weight_label_diff,
									std::vector<int>& weight_label_diff_left_vertex);

	/** square matrix saving the matching results */
	match_matrix match_;

	/** square matrix saving the edge weights */
	weight_matrix weight_;

	/** the left vertices of matched vertices
	 * on the right (in current match)
	 * used to map matched left vertex given right vertex
	 */
	std::vector<int> matched_on_left_;

	/** the right vertices of matched vertices
	 * on the right (in current match)
	 * used to map matched right vertex given left vertex
	 */
	std::vector<int> matched_on_right_;
	/** number of vertices in the half of bipartite graph */
	int size_;
	
	/** sum of edge weights in final match */
	double weight_sum_;

	/** definition for node that is not assigned / matched in algorithm */
	static const int kUnexist;
};

} // end of namespace testing_ai_default

} // end of namespace ai

#endif /* defined(AI_TESTING_MAX_WEIGHT_MATCH_HPP) */
