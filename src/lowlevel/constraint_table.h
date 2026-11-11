#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_set>

#include "core/types.h"

namespace mapf {

// The constraints that apply to one agent at one constraint-tree node, indexed
// for the low-level search. Semantics follow docs/mapf-spec.md section 6.
class ConstraintTable {
public:
    ConstraintTable(int agent, int goal);

    // Adds `constraint` when it belongs to this table's agent and ignores it
    // otherwise, so a caller can feed a whole chain of constraint-tree nodes.
    // Throws std::invalid_argument for a negative timestep, or for an edge
    // constraint with timestep 0 (no move arrives at time 0).
    void add(const Constraint& constraint);

    int agent() const { return agent_; }
    int goal() const { return goal_; }

    // True when the agent may not be on `cell` at `timestep`.
    bool is_vertex_blocked(int cell, int timestep) const;

    // True when the agent may not move from `from` to `to`, arriving at `timestep`.
    bool is_edge_blocked(int from, int to, int timestep) const;

    // Largest timestep of any constraint, or -1 when there are none. Nothing
    // is blocked after this time.
    int latest_time() const { return latest_time_; }

    // Earliest timestep at which the agent may stop on its goal for good:
    // one more than the latest vertex constraint on the goal, or 0 without one.
    int goal_hold_time() const { return goal_hold_time_; }

private:
    struct EdgeKey {
        int from;
        int to;
        int timestep;
        bool operator==(const EdgeKey& other) const {
            return from == other.from && to == other.to && timestep == other.timestep;
        }
    };
    struct EdgeKeyHash {
        std::size_t operator()(const EdgeKey& key) const;
    };

    int agent_;
    int goal_;
    int latest_time_ = -1;
    int goal_hold_time_ = 0;
    std::unordered_set<std::uint64_t> vertex_blocks_;
    std::unordered_set<EdgeKey, EdgeKeyHash> edge_blocks_;
};

}  // namespace mapf
