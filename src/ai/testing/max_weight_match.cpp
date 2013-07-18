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
 * Max weight hungary match algorithm implementation
 */

#include <cassert>
#include <algorithm>
#include <queue>
#include <limits>
#include "max_weight_match.hpp"

namespace ai {

namespace testing_ai_default {

const int max_assignment::kUnexist = -1;

max_assignment::max_assignment(const cost_matrix& cost_mat) {
	assert(!cost_mat.empty());
	int left_size = cost_mat.size();
	int right_size = cost_mat[0].size();
	size_ = left_size > right_size ? left_size : right_size;

	// 2D - cost matrix may have different dimension sizes,
	// we adjust it to same dimension size for later
	// computation
	if (left_size != right_size) {
		for (int i = 0; i < size_; ++i) {
			if (i < left_size) {
				std::vector<int> tmp = cost_mat[i];
				for (int j = tmp.size(); j < size_; ++j) {
					tmp.push_back(0);
				}
				cost_.push_back(tmp);
			}
			else {
				cost_.push_back(std::vector<int>(size_, 0));
			}
		}
	}
	else {
		cost_ = cost_mat;
	}

	// initialization
	matched_on_left_.reserve(size_);
	matched_on_right_.reserve(size_);
	for (int i = 0; i < size_; ++i) {
		matched_on_left_.push_back(kUnexist);
		matched_on_right_.push_back(kUnexist);
	}

	// call weighted hungary algorithm
	// to compute max weight match
	weighted_hungary_routine();

	// save results
	for (int left = 0; left < size_; ++left) {
		match_.push_back(std::vector<bool>());
		for (int right = 0; right < size_; ++right) {
			match_[left].push_back(false);
		}
		int matched_right = matched_on_right_[left];
		assert(matched_right < size_);
		match_[left][matched_right] = true;
	}
}

void max_assignment::update_labels(const std::vector<bool>& right_ismatched,
								   const std::vector<bool>& left_ismatched,
								   std::vector<int>& cost_label_diff,
								   std::vector<int>& left_label,
								   std::vector<int>& right_label) {
	// find minimum delta for all right vertices
	// that are not matched.
	// delta = min_right {cost_label_diff_}
	int delta = std::numeric_limits<int>::max();
	for (int right = 0; right < size_; ++right) {
		if (!right_ismatched[right] &&
				cost_label_diff[right] < delta) {
			delta = cost_label_diff[right];
		}
	}

	// update labels and cost_label_diff vector
	for (int right = 0; right < size_; ++right) {
		if (right_ismatched[right]) {
			right_label[right] += delta;
		}
		else {
			// update cost label diff since label of
			// left vertices are changed
			cost_label_diff[right] -= delta;
		}
	}

	for (int left = 0; left < size_; ++left) {
		if(left_ismatched[left]) {
			left_label[left] -= delta;
		}
	}
}

void max_assignment::add_edge_to_equality_graph(const int left,
		const int prev_left_vertex,
		const std::vector<int>& left_label,
		const std::vector<int>& right_label,
		std::vector<bool>& left_ismatched,
		std::vector<int>& prev_left,
		std::vector<int>& cost_label_diff,
		std::vector<int>& cost_label_diff_left_vertex) {
	// we need to add left vertex only and
	// right vertex of the edge can be found
	// through matched_on_right
	left_ismatched[left] = true;
	prev_left[left] = prev_left_vertex;

	// update cost_label_diff as equality graph
	// is changed
	for (int right = 0; right < size_; ++right) {
		int diff = left_label[left] + right_label[right] - cost_[left][right];
		if (diff < cost_label_diff[right]) {
			cost_label_diff[right] = diff;
			cost_label_diff_left_vertex[right] = left;
		}
	}
}

void max_assignment::do_augment(std::vector<bool>& left_ismatched,
								std::vector<bool>& right_ismatched,
								std::vector<int>& prev_left,
								std::vector<int>& cost_label_diff,
								std::vector<int>& cost_label_diff_left_vertex,
								int& matched_size,
								std::vector<int>& left_label,
								std::vector<int>& right_label) {
	// the queue is used in BFS later looking for
	// augmenting path
	std::queue<int> q;

	// reset matched vertices status
	std::fill(left_ismatched.begin(), left_ismatched.end(), false);
	std::fill(right_ismatched.begin(), right_ismatched.end(), false);
	std::fill(prev_left.begin(), prev_left.end(), kUnexist);

	// we find an unmatched vertex on left
	// and set it as the root of alternating
	// tree we are going to build
	int left_root;
	for (int left = 0; left < size_; ++left) {
		if (matched_on_right_[left] == kUnexist) {
			left_root = left;
			left_ismatched[left_root] = true;
			prev_left[left_root] = kUnexist;
			break;
		}
	}

	// reset cost_label_diff_
	for (int right = 0; right < size_; ++right) {
		cost_label_diff[right] = left_label[left_root] +
								 right_label[right] - cost_[left_root][right];
	}
	std::fill(cost_label_diff_left_vertex.begin(),
			  cost_label_diff_left_vertex.end(), left_root);

	// Main routing looking for augmenting path
	// in equality graph built. If BFS fails, we update
	// values and equality graph, and search again.
	q.push(left_root);
	left_ismatched[left_root] = true;
	bool is_augmenting_path_found = false;
	int left, right;
	while (!is_augmenting_path_found) {
		// BFS routing looking for augmenting path
		while (!q.empty()) {
			left = q.front();
			q.pop();
			for (right = 0; right < size_; ++right) {
				if (cost_[left][right] == left_label[left] + right_label[right]
						&& !right_ismatched[right]) {
					if (matched_on_left_[right] == kUnexist) {
						// right vertex is not matched with left vertex
						// we find an augmenting path
						is_augmenting_path_found = true;
						break;
					}
					// right vertex is matched with left vertex
					// add left-right to alternating tree
					// add matched left vertex to queue
					right_ismatched[right] = true;
					add_edge_to_equality_graph(matched_on_left_[right],
											   left,
											   left_label,
											   right_label,
											   left_ismatched,
											   prev_left,
											   cost_label_diff,
											   cost_label_diff_left_vertex);
					q.push(matched_on_left_[right]);
				}
			}

			if (is_augmenting_path_found) {
				break;
			}
		}

		if (!is_augmenting_path_found) {
			// augmenting path is not found
			// we update labels and equality graph
			update_labels(right_ismatched,
						  left_ismatched,
						  cost_label_diff,
						  left_label,
						  right_label);

			// search to see if we find new edges.
			for (right = 0; right < size_; ++right) {
				if (!right_ismatched[right]
						&& cost_label_diff[right] == 0) {
					if (matched_on_left_[right] == kUnexist) {
						// right vertex is not matched with left vertex
						// we find an augmenting path
						left = cost_label_diff_left_vertex[right];
						is_augmenting_path_found = true;
						break;
					}
					// right vertex is matched with left vertex
					right_ismatched[right] = true;

					// only add to tree if the left vertex is not matched
					// (if the left is matched, the edge should have
					// been added in BFS)
					if (!left_ismatched[matched_on_left_[right]]) {
						add_edge_to_equality_graph(matched_on_left_[right],
												   cost_label_diff_left_vertex[right],
												   left_label,
												   right_label,
												   left_ismatched,
												   prev_left,
												   cost_label_diff,
												   cost_label_diff_left_vertex);
						q.push(matched_on_left_[right]);
					}
				}
			}

			if (is_augmenting_path_found) {
				break;
			}
		} // end of if (!is_augmenting_path_found)
	} // end of while (!is_augmenting_path_found)

	if (is_augmenting_path_found) {
		// reverse the augmenting path by changing the
		// matched status
		while (left != kUnexist) {
			int prev_right = matched_on_right_[left];
			matched_on_right_[left] = right;
			matched_on_left_[right] = left;
			right = prev_right;
			left = prev_left[left];
		};

		// increase matched size, return if all
		// vertices are matched
		if (++matched_size == size_) return;

		// tail recursively do augment again
		do_augment(left_ismatched,
				   right_ismatched,
				   prev_left,
				   cost_label_diff,
				   cost_label_diff_left_vertex,
				   matched_size,
				   left_label,
				   right_label);
	}
}

void max_assignment::weighted_hungary_routine() {
	/** the label values of vertices on the left */
	std::vector<int> left_label;

	/** the label values of vertices on the right */
	std::vector<int> right_label;

	// left labels are initialized with max cost
	// among all outgoing edges
	// right labels are initialized with 0
	left_label.reserve(size_);
	right_label.reserve(size_);
	for (int i = 0; i < size_; ++i) {
		int maxcost = 0;
		for (int j = 0; j < size_; ++j) {
			maxcost = std::max(maxcost, cost_[i][j]);
		}
		left_label.push_back(maxcost);
		right_label.push_back(0);
	}

	/** the status if vertices on the left are matched */
	std::vector<bool> left_ismatched;

	/** the status if vertices on the right are matched */
	std::vector<bool> right_ismatched;

	// all vertices are set to unmatched
	left_ismatched.reserve(size_);
	right_ismatched.reserve(size_);
	for (int i = 0; i < size_; ++i) {
		left_ismatched.push_back(false);
		right_ismatched.push_back(false);
	}

	/** the difference between label value sum and cost
	 * of an edge. Helper vector in improving look-up
	 * speed in finding minimum difference among edges.
	 * Indexed by right vertex of the edge
	 */
	std::vector<int> cost_label_diff;

	/** the corresponding left vertices of cost_label_diff_*/
	std::vector<int> cost_label_diff_left_vertex;

	// initialize cost_label_diff_
	for (int right = 0; right < size_; ++right) {
		cost_label_diff.push_back(0);
		cost_label_diff_left_vertex.push_back(0);
	}

	/** the previous left vertex of current left vertex
	 * on alternating path. Required in augmenting path
	 */
	std::vector<int> prev_left;

	// init prev_left to unexist status
	for (int i = 0; i < size_; ++i) {
		prev_left.push_back(kUnexist);
	}

	int matched_size = 0;

	// begin augment process
	do_augment(left_ismatched,
			   right_ismatched,
			   prev_left,
			   cost_label_diff,
			   cost_label_diff_left_vertex,
			   matched_size,
			   left_label,
			   right_label);
}

} // end of namespace testing_ai_default

} // end of namespace ai