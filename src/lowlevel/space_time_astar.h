#pragma once

#include <array>

#include "core/grid.h"
#include "core/types.h"
#include "lowlevel/constraint_table.h"
#include "lowlevel/heuristic_table.h"

namespace mapf {

// Cells an agent may occupy one timestep after standing on some cell: the
// passable neighbours in the order up, down, left, right, then the cell itself
// (waiting), with every move that a constraint forbids removed.
struct Successors {
    std::array<int, 5> cells{};
    int count = 0;

    const int* begin() const { return cells.data(); }
    const int* end() const { return cells.data() + count; }
};

// Successors of `cell` for a move that arrives at `arrival_time`. Shared by
// every low-level search so they all apply constraints the same way.
Successors constrained_successors(const Grid& grid, const ConstraintTable& constraints, int cell,
                                  int arrival_time);

// Space-Time A*: finds a minimum-cost path for one agent from `start` to
// `heuristic.goal()` that satisfies `constraints`.
//
// - Solved: `path` is in the normal form of docs/mapf-spec.md section 3,
//   `cost == path.size() - 1`, and `lower_bound == cost`.
// - NoPath: no path satisfies the constraints. The search always terminates,
//   because all states later than the last constraint are merged per cell.
// - Timeout: `deadline` passed; `lower_bound` is the largest f-value expanded.
//
// Deterministic: equal inputs give the same path. Throws std::invalid_argument
// when `constraints.goal()` differs from `heuristic.goal()` or `start` is not
// a passable cell.
SearchResult find_path(const Grid& grid, const HeuristicTable& heuristic, int start,
                       const ConstraintTable& constraints, Deadline deadline);

}  // namespace mapf
