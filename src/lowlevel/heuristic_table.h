#pragma once

#include <limits>
#include <vector>

#include "core/grid.h"

namespace mapf {

// Exact shortest-path distance from every cell to one goal on the static grid,
// ignoring all other agents and all constraints. Because it is exact on the
// static grid, it is an admissible and consistent heuristic for any search
// that only adds constraints.
class HeuristicTable {
public:
    // Distance reported for obstacles and for cells that cannot reach the goal.
    static constexpr int kUnreachable = std::numeric_limits<int>::max();

    // Runs one breadth-first search from `goal`. Throws std::invalid_argument
    // when `goal` is outside the grid or on an obstacle.
    HeuristicTable(const Grid& grid, int goal);

    int goal() const { return goal_; }
    int distance(int cell) const { return distance_[cell]; }
    bool reachable(int cell) const { return distance_[cell] != kUnreachable; }

private:
    int goal_;
    std::vector<int> distance_;
};

}  // namespace mapf
