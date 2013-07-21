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

max_assignment::max_assignment(const weight_matrix& weight_mat) {
	assert(!weight_mat.empty());
	int left_size = weight_mat.size();
	int right_size = weight_mat[0].size();
	size_ = left_size > right_size ? left_size : right_size;

	// 2D - weight matrix may have different dimension sizes,
	// we adjust it to same dimension size for later
	// computation
	if (left_size != right_size) {
		for (int i = 0; i < size_; ++i) {
			if (i < left_size) {
				std::vector<int> tmp = weight_mat[i];
				for (int j = tmp.size(); j < size_; ++j) {
					tmp.push_back(0);
				}
				weight_.push_back(tmp);
			}
			else {
				weight_.push_back(std::vector<int>(size_, 0));
			}
		}
	}
	else {
		weight_ = weight_mat;
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
	weight_sum_ = 0;
	for (int left = 0; left < size_; ++left) {
		match_.push_back(std::vector<bool>());
		for (int right = 0; right < size_; ++right) {
			match_[left].push_back(false);
		}
		int matched_right = matched_on_right_[left];
		assert(matched_right < size_);
		match_[left][matched_right] = true;
		weight_sum_ += weight_[left][matched_right];
	}
}

void max_assignment::update_labels(const std::vector<bool>& right_ismatched,
								   const std::vector<bool>& left_ismatched,
								   std::vector<int>& weight_label_diff,
								   std::vector<int>& left_label,
								   std::vector<int>& right_label) {
	// find minimum delta for all right vertices
	// that are not matched.
	// delta = min_right {weight_label_diff_}
	int delta = std::numeric_limits<int>::max();
	for (int right = 0; right < size_; ++right) {
		if (!right_ismatched[right] &&
				weight_label_diff[right] < delta) {
			delta = weight_label_diff[right];
		}
	}

	// update labels and weight_label_diff vector
	for (int right = 0; right < size_; ++right) {
		if (right_ismatched[right]) {
			right_label[right] += delta;
		}
		else {
			// update weight label diff since label of
			// left vertices are changed
			weight_label_diff[right] -= delta;
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
		std::vector<int>& weight_label_diff,
		std::vector<int>& weight_label_diff_left_vertex) {
	// we need to add left vertex only and
	// right vertex of the edge can be found
	// through matched_on_right
	left_ismatched[left] = true;
	prev_left[left] = prev_left_vertex;

	// update weight_label_diff as equality graph
	// is changed
	for (int right = 0; right < size_; ++right) {
		int diff = left_label[left] + right_label[right] - weight_[left][right];
		if (diff < weight_label_diff[right]) {
			weight_label_diff[right] = diff;
			weight_label_diff_left_vertex[right] = left;
		}
	}
}

void max_assignment::do_augment(std::vector<bool>& left_ismatched,
								std::vector<bool>& right_ismatched,
								std::vector<int>& prev_left,
								std::vector<int>& weight_label_diff,
								std::vector<int>& weight_label_diff_left_vertex,
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

	// reset weight_label_diff_
	for (int right = 0; right < size_; ++right) {
		weight_label_diff[right] = left_label[left_root] +
								 right_label[right] - weight_[left_root][right];
	}
	std::fill(weight_label_diff_left_vertex.begin(),
			  weight_label_diff_left_vertex.end(), left_root);

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
				if (weight_[left][right] == left_label[left] + right_label[right]
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
											   weight_label_diff,
											   weight_label_diff_left_vertex);
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
						  weight_label_diff,
						  left_label,
						  right_label);

			// search to see if we find new edges.
			for (right = 0; right < size_; ++right) {
				if (!right_ismatched[right]
						&& weight_label_diff[right] == 0) {
					if (matched_on_left_[right] == kUnexist) {
						// right vertex is not matched with left vertex
						// we find an augmenting path
						left = weight_label_diff_left_vertex[right];
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
												   weight_label_diff_left_vertex[right],
												   left_label,
												   right_label,
												   left_ismatched,
												   prev_left,
												   weight_label_diff,
												   weight_label_diff_left_vertex);
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
				   weight_label_diff,
				   weight_label_diff_left_vertex,
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

	// left labels are initialized with max weight
	// among all outgoing edges
	// right labels are initialized with 0
	left_label.reserve(size_);
	right_label.reserve(size_);
	for (int i = 0; i < size_; ++i) {
		int maxweight = 0;
		for (int j = 0; j < size_; ++j) {
			maxweight = std::max(maxweight, weight_[i][j]);
		}
		left_label.push_back(maxweight);
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

	/** the difference between label value sum and weight
	 * of an edge. Helper vector in improving look-up
	 * speed in finding minimum difference among edges.
	 * Indexed by right vertex of the edge
	 */
	std::vector<int> weight_label_diff;

	/** the corresponding left vertices of weight_label_diff_*/
	std::vector<int> weight_label_diff_left_vertex;

	// initialize weight_label_diff_
	for (int right = 0; right < size_; ++right) {
		weight_label_diff.push_back(0);
		weight_label_diff_left_vertex.push_back(0);
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
			   weight_label_diff,
			   weight_label_diff_left_vertex,
			   matched_size,
			   left_label,
			   right_label);
}

} // end of namespace testing_ai_default

} // end of namespace ai