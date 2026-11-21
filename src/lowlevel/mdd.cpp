#include "lowlevel/mdd.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "lowlevel/space_time_astar.h"

namespace mapf {

Mdd build_mdd(const Grid& grid, const HeuristicTable& heuristic, int start,
              const ConstraintTable& constraints, int cost) {
    const int goal = heuristic.goal();
    if (constraints.goal() != goal) {
        throw std::invalid_argument("constraint table and heuristic table have different goals");
    }
    if (start < 0 || start >= grid.size() || !grid.passable(start)) {
        throw std::invalid_argument("start must be a passable cell");
    }
    if (cost < 0) {
        throw std::invalid_argument("MDD cost must not be negative");
    }

    Mdd mdd;
    if (!heuristic.reachable(start) || heuristic.distance(start) > cost ||
        cost < constraints.goal_hold_time() || constraints.is_vertex_blocked(start, 0)) {
        return mdd;
    }

    // Forward pass: cells reachable at each timestep that can still reach the
    // goal in the remaining time on the static grid.
    std::vector<std::vector<int>> forward(static_cast<std::size_t>(cost) + 1);
    forward[0].push_back(start);
    std::vector<std::uint8_t> seen(static_cast<std::size_t>(grid.size()), 0);
    for (int t = 0; t < cost; ++t) {
        std::vector<int>& next_layer = forward[static_cast<std::size_t>(t) + 1];
        for (const int cell : forward[static_cast<std::size_t>(t)]) {
            for (const int next : constrained_successors(grid, constraints, cell, t + 1)) {
                // Successors of a cell that reaches the goal reach it too, so
                // the distance is finite.
                if (seen[static_cast<std::size_t>(next)] == 0 &&
                    heuristic.distance(next) <= cost - (t + 1)) {
                    seen[static_cast<std::size_t>(next)] = 1;
                    next_layer.push_back(next);
                }
            }
        }
        for (const int cell : next_layer) {
            seen[static_cast<std::size_t>(cell)] = 0;
        }
        if (next_layer.empty()) {
            return mdd;
        }
    }

    // The static distance ignores constraints, so the forward layers can hold
    // dead ends. Backward pass: keep a cell only when one of its moves leads
    // to a cell kept in the next layer.
    std::vector<int>& last_layer = forward[static_cast<std::size_t>(cost)];
    if (std::find(last_layer.begin(), last_layer.end(), goal) == last_layer.end()) {
        return mdd;
    }
    last_layer.assign(1, goal);
    for (int t = cost - 1; t >= 0; --t) {
        for (const int cell : forward[static_cast<std::size_t>(t) + 1]) {
            seen[static_cast<std::size_t>(cell)] = 1;
        }
        std::vector<int>& layer = forward[static_cast<std::size_t>(t)];
        std::vector<int> kept;
        for (const int cell : layer) {
            for (const int next : constrained_successors(grid, constraints, cell, t + 1)) {
                if (seen[static_cast<std::size_t>(next)] != 0) {
                    kept.push_back(cell);
                    break;
                }
            }
        }
        for (const int cell : forward[static_cast<std::size_t>(t) + 1]) {
            seen[static_cast<std::size_t>(cell)] = 0;
        }
        layer = std::move(kept);
    }

    for (std::vector<int>& layer : forward) {
        std::sort(layer.begin(), layer.end());
    }
    mdd.layers = std::move(forward);
    return mdd;
}

}  // namespace mapf
