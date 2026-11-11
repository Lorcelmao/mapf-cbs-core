#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

// Shared data types of the MAPF model. Their meaning is fixed by docs/mapf-spec.md.

namespace mapf {

// path[t] is the cell an agent occupies at timestep t; path[0] is its start.
// After the last entry the agent stays on that cell forever.
using Path = std::vector<int>;

enum class ConstraintType { Vertex, Edge };

// Forbids one agent from one cell or one move at one timestep.
// Every timestep is an arrival time: an edge constraint (from, to, timestep)
// forbids leaving `from` at timestep - 1 and arriving at `to` at `timestep`.
struct Constraint {
    int agent;
    ConstraintType type;
    int from;      // Edge: cell the agent would leave. Vertex: unused, set to -1.
    int to;        // Vertex: the forbidden cell. Edge: cell the agent would enter.
    int timestep;
};

enum class SearchStatus { Solved, NoPath, Timeout };

// Result of one low-level search for a single agent.
struct SearchResult {
    Path path;
    // Timestep at which the agent reaches its goal for good.
    int cost = 0;
    // Smallest f-value left in OPEN when the search stopped. An optimal search
    // reports lower_bound == cost; a bounded-suboptimal search may report
    // lower_bound < cost, so the two fields must never be merged.
    int lower_bound = 0;
    std::uint64_t expanded = 0;
    std::uint64_t generated = 0;
    double runtime_s = 0.0;
    SearchStatus status = SearchStatus::NoPath;
};

// Wall-clock instant after which a search must stop and report Timeout.
using Deadline = std::chrono::steady_clock::time_point;

}  // namespace mapf
