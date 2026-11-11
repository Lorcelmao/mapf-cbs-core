#pragma once

#include <vector>

#include "core/grid.h"
#include "core/types.h"

namespace mapf_test {

// Slow but obviously correct bounded BFS over (cell, time) layers.
inline int reference_min_cost(const mapf::Grid& grid, int start, int goal,
                              const std::vector<mapf::Constraint>& constraints) {
    const int cell_count = grid.size();
    const std::size_t n = static_cast<std::size_t>(cell_count);

    // Largest timestep used by any constraint; after it nothing new can be forbidden.
    int max_time = -1;
    for (const mapf::Constraint& c : constraints) {
        if (c.timestep > max_time) max_time = c.timestep;
    }

    // The agent rests on the goal forever, so it may only stop at T if no vertex
    // constraint on the goal exists at any time >= T.
    int hold = 0;
    for (const mapf::Constraint& c : constraints) {
        if (c.type == mapf::ConstraintType::Vertex && c.to == goal && c.timestep + 1 > hold) {
            hold = c.timestep + 1;
        }
    }

    auto vertex_forbidden = [&constraints](int cell, int t) {
        for (const mapf::Constraint& c : constraints) {
            if (c.type == mapf::ConstraintType::Vertex && c.to == cell && c.timestep == t) {
                return true;
            }
        }
        return false;
    };

    // Edge constraints are directed: only the departure -> arrival order is checked.
    auto edge_forbidden = [&constraints](int from, int to, int t) {
        for (const mapf::Constraint& c : constraints) {
            if (c.type == mapf::ConstraintType::Edge && c.from == from && c.to == to &&
                c.timestep == t) {
                return true;
            }
        }
        return false;
    };

    // Sitting on the start at time 0 is not negotiable, so the instance is dead.
    if (vertex_forbidden(start, 0)) return -1;

    std::vector<char> current(n, 0);
    std::vector<char> next(n, 0);
    current[static_cast<std::size_t>(start)] = 1;

    // One full sweep of the grid length is enough to reach any cell, the rest is slack
    // for waiting out constraints.
    const int horizon = max_time + cell_count + 1;

    for (int t = 0; t <= horizon; ++t) {
        if (t >= hold && current[static_cast<std::size_t>(goal)] != 0) return t;
        if (t == horizon) break;  // no layer to expand, no earlier stop was valid

        next.assign(n, 0);

        for (int u = 0; u < cell_count; ++u) {
            if (current[static_cast<std::size_t>(u)] == 0) continue;

            // Waiting keeps the same cell; only constraints can rule it out.
            if (!vertex_forbidden(u, t + 1) && !edge_forbidden(u, u, t + 1)) {
                next[static_cast<std::size_t>(u)] = 1;
            }

            const mapf::Neighbors nb = grid.neighbors(u);
            for (const int* it = nb.begin(); it != nb.end(); ++it) {
                const int v = *it;
                if (vertex_forbidden(v, t + 1)) continue;
                if (edge_forbidden(u, v, t + 1)) continue;
                next[static_cast<std::size_t>(v)] = 1;
            }
        }

        current.swap(next);
    }

    return -1;
}

}  // namespace mapf_test
