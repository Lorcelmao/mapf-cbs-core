#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core/types.h"

namespace mapf {

enum class SolveStatus {
    Solved,      // conflict-free paths for all agents were found
    Timeout,     // the time limit passed first
    NoSolution,  // proven unsolvable, e.g. an agent cannot reach its goal
    Error        // the run failed; see SolveResult::error
};

// Outcome of one multi-agent solver run. Every solver (CBS, ECBS) fills the
// same structure so the executable can report them uniformly.
struct SolveResult {
    SolveStatus status = SolveStatus::Error;
    // Human-readable reason when status is Error or NoSolution, else empty.
    std::string error;

    // One path per agent, in agent order. Empty unless status is Solved.
    std::vector<Path> paths;
    std::optional<int> soc;
    std::optional<int> makespan;

    // Lower bounds on the optimal sum-of-costs. Unset when not computed.
    std::optional<int> lb_independent;  // sum of single-agent shortest paths
    std::optional<int> lb_frontier;     // smallest bound on the search frontier
    std::optional<int> lb_certified;    // best bound known to be valid
    // Which bound lb_certified is: "optimal", "independent", "frontier", or empty.
    std::string lb_source;
    std::optional<bool> frontier_consistent;

    std::uint64_t hl_expanded = 0;   // constraint-tree nodes expanded
    std::uint64_t hl_generated = 0;  // constraint-tree nodes generated
    std::uint64_t ll_calls = 0;      // low-level searches run
    std::uint64_t ll_expanded = 0;   // low-level states expanded, all calls
    std::uint64_t ll_generated = 0;  // low-level states generated, all calls

    double runtime_s = 0.0;           // whole solve
    double runtime_lowlevel_s = 0.0;  // inside low-level searches
    double runtime_detect_s = 0.0;    // inside conflict detection
};

}  // namespace mapf
