#include "lowlevel/space_time_astar.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace mapf {

namespace {

// One generated search state. `parent` indexes the node it was reached from,
// or is -1 for the start.
struct Node {
    int cell;
    int time;
    int parent;
};

struct OpenEntry {
    int f;
    int g;
    int cell;
    int node;
};

// Priority order of OPEN: smallest f first; among equal f the deepest state
// (largest g) first, which reaches the goal sooner; then smallest cell and
// oldest node, so that the order is total and the search is deterministic.
struct LowerPriority {
    bool operator()(const OpenEntry& a, const OpenEntry& b) const {
        if (a.f != b.f) {
            return a.f > b.f;
        }
        if (a.g != b.g) {
            return a.g < b.g;
        }
        if (a.cell != b.cell) {
            return a.cell > b.cell;
        }
        return a.node > b.node;
    }
};

std::uint64_t state_key(int cell, int merged_time) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cell)) << 32) |
           static_cast<std::uint32_t>(merged_time);
}

}  // namespace

Successors constrained_successors(const Grid& grid, const ConstraintTable& constraints, int cell,
                                  int arrival_time) {
    Successors result;
    for (const int next : grid.neighbors(cell)) {
        if (!constraints.is_vertex_blocked(next, arrival_time) &&
            !constraints.is_edge_blocked(cell, next, arrival_time)) {
            result.cells[result.count++] = next;
        }
    }
    if (!constraints.is_vertex_blocked(cell, arrival_time) &&
        !constraints.is_edge_blocked(cell, cell, arrival_time)) {
        result.cells[result.count++] = cell;
    }
    return result;
}

SearchResult find_path(const Grid& grid, const HeuristicTable& heuristic, int start,
                       const ConstraintTable& constraints, Deadline deadline) {
    const auto started = std::chrono::steady_clock::now();
    const int goal = heuristic.goal();
    if (constraints.goal() != goal) {
        throw std::invalid_argument("constraint table and heuristic table have different goals");
    }
    if (start < 0 || start >= grid.size() || !grid.passable(start)) {
        throw std::invalid_argument("start must be a passable cell");
    }

    SearchResult result;
    const auto finish = [&](SearchStatus status) {
        result.status = status;
        result.runtime_s =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
        return result;
    };

    // Constraints only remove moves, so a goal that is unreachable on the
    // static grid stays unreachable. A constraint on the start at time 0 can
    // never be satisfied either.
    if (!heuristic.reachable(start) || constraints.is_vertex_blocked(start, 0)) {
        return finish(SearchStatus::NoPath);
    }

    // From this time on no constraint applies, so the cheapest way to finish
    // from a cell no longer depends on the time. All such states of one cell
    // are merged into one, which makes the state space finite: the search
    // terminates even when no path exists.
    const int merge_time = constraints.latest_time() + 1;
    const int goal_hold_time = constraints.goal_hold_time();

    std::vector<Node> nodes;
    std::priority_queue<OpenEntry, std::vector<OpenEntry>, LowerPriority> open;
    // Smallest g found so far for each (cell, merged time) state.
    std::unordered_map<std::uint64_t, int> best_g;

    nodes.push_back(Node{start, 0, -1});
    open.push(OpenEntry{heuristic.distance(start), 0, start, 0});
    best_g.emplace(state_key(start, std::min(0, merge_time)), 0);
    result.generated = 1;
    result.lower_bound = heuristic.distance(start);

    while (!open.empty()) {
        if ((result.expanded & 1023u) == 0 && std::chrono::steady_clock::now() >= deadline) {
            return finish(SearchStatus::Timeout);
        }

        const OpenEntry current = open.top();
        open.pop();
        // A cheaper way to the same merged state was found after this entry
        // was queued.
        if (best_g.at(state_key(current.cell, std::min(current.g, merge_time))) < current.g) {
            continue;
        }
        // The heuristic is consistent, so popped f-values never decrease.
        result.lower_bound = current.f;

        if (current.cell == goal && current.g >= goal_hold_time) {
            result.cost = current.g;
            for (int index = current.node; index != -1; index = nodes[index].parent) {
                result.path.push_back(nodes[index].cell);
            }
            std::reverse(result.path.begin(), result.path.end());
            return finish(SearchStatus::Solved);
        }

        ++result.expanded;
        const int next_time = current.g + 1;
        for (const int next : constrained_successors(grid, constraints, current.cell, next_time)) {
            const std::uint64_t key = state_key(next, std::min(next_time, merge_time));
            const auto [entry, inserted] = best_g.emplace(key, next_time);
            if (!inserted) {
                if (entry->second <= next_time) {
                    continue;
                }
                entry->second = next_time;
            }
            // Every cell reached from a reachable start can reach the goal on
            // the static grid, so its distance is finite and f cannot overflow.
            nodes.push_back(Node{next, next_time, current.node});
            open.push(OpenEntry{next_time + heuristic.distance(next), next_time, next,
                                static_cast<int>(nodes.size()) - 1});
            ++result.generated;
        }
    }
    return finish(SearchStatus::NoPath);
}

}  // namespace mapf
