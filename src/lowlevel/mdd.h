#pragma once

#include <vector>

#include "core/grid.h"
#include "lowlevel/constraint_table.h"
#include "lowlevel/heuristic_table.h"

namespace mapf {

// Multi-valued decision diagram of one agent: all the ways it can walk from its
// start to its goal in exactly `cost` timesteps while satisfying its constraints.
//
// Built for the agent's optimal cost, it holds exactly the agent's optimal
// paths, which is what conflict classification needs: a conflict at timestep t
// is unavoidable for the agent when layer t contains only the conflicting cell.
// Built for a larger cost, it also holds walks that pause on the goal early.
struct Mdd {
    // layers[t] lists, in increasing order, the cells the agent occupies at
    // timestep t on at least one such walk. No layers means no walk exists.
    std::vector<std::vector<int>> layers;

    bool empty() const { return layers.empty(); }
    int cost() const { return static_cast<int>(layers.size()) - 1; }
    int width(int timestep) const {
        return static_cast<int>(layers[static_cast<std::size_t>(timestep)].size());
    }
};

// Builds the MDD of the agent that owns `constraints`, for walks of exactly
// `cost` steps from `start` to `heuristic.goal()`. Returns an empty MDD when no
// such walk exists, including when `cost` is below `constraints.goal_hold_time()`
// so the agent could not stay on the goal afterwards.
//
// Runs in O(cost * |V|) time and memory. Throws std::invalid_argument when
// `cost` is negative, `start` is not a passable cell, or the two tables have
// different goals.
Mdd build_mdd(const Grid& grid, const HeuristicTable& heuristic, int start,
              const ConstraintTable& constraints, int cost);

}  // namespace mapf
