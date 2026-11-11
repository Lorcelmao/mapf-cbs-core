#include "lowlevel/heuristic_table.h"

#include <queue>
#include <stdexcept>

namespace mapf {

HeuristicTable::HeuristicTable(const Grid& grid, int goal)
    : goal_(goal), distance_(static_cast<std::size_t>(grid.size()), kUnreachable) {
    if (goal < 0 || goal >= grid.size()) {
        throw std::invalid_argument("HeuristicTable: goal is outside the grid");
    }
    if (!grid.passable(goal)) {
        throw std::invalid_argument("HeuristicTable: goal is on an obstacle");
    }

    std::queue<int> frontier;
    distance_[static_cast<std::size_t>(goal)] = 0;
    frontier.push(goal);

    while (!frontier.empty()) {
        const int cell = frontier.front();
        frontier.pop();
        const int next_distance = distance_[static_cast<std::size_t>(cell)] + 1;
        for (const int neighbour : grid.neighbors(cell)) {
            // kUnreachable doubles as the "not visited yet" marker.
            if (distance_[static_cast<std::size_t>(neighbour)] == kUnreachable) {
                distance_[static_cast<std::size_t>(neighbour)] = next_distance;
                frontier.push(neighbour);
            }
        }
    }
}

}  // namespace mapf
