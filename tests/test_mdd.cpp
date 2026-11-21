#include <doctest.h>

#include <chrono>
#include <cstdint>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/grid.h"
#include "core/types.h"
#include "lowlevel/constraint_table.h"
#include "lowlevel/heuristic_table.h"
#include "lowlevel/mdd.h"
#include "lowlevel/space_time_astar.h"

using mapf::Constraint;
using mapf::ConstraintTable;
using mapf::ConstraintType;
using mapf::Grid;
using mapf::HeuristicTable;
using mapf::Mdd;

namespace {

const int kAgent = 0;
using Layers = std::vector<std::vector<int>>;

Constraint vertex(int cell, int timestep) {
    return Constraint{kAgent, ConstraintType::Vertex, -1, cell, timestep};
}

Constraint edge(int from, int to, int timestep) {
    return Constraint{kAgent, ConstraintType::Edge, from, to, timestep};
}

ConstraintTable table_of(int goal, const std::vector<Constraint>& constraints) {
    ConstraintTable table(kAgent, goal);
    for (const Constraint& constraint : constraints) {
        table.add(constraint);
    }
    return table;
}

Mdd mdd_of(const Grid& grid, int start, int goal, const std::vector<Constraint>& constraints,
           int cost) {
    const HeuristicTable heuristic(grid, goal);
    return mapf::build_mdd(grid, heuristic, start, table_of(goal, constraints), cost);
}

// True when the agent may be on `cell` at `timestep`, and when it may move
// `from` -> `to` arriving at `timestep`, by scanning the raw constraint list.
bool move_allowed(const std::vector<Constraint>& constraints, int from, int to, int timestep) {
    for (const Constraint& constraint : constraints) {
        if (constraint.timestep != timestep) {
            continue;
        }
        if (constraint.type == ConstraintType::Vertex && constraint.to == to) {
            return false;
        }
        if (constraint.type == ConstraintType::Edge && constraint.from == from &&
            constraint.to == to) {
            return false;
        }
    }
    return true;
}

// Enumerates every walk of exactly `cost` steps and records the cells of the
// walks that end on the goal. Exponential, so only for tiny instances.
void enumerate_walks(const Grid& grid, int goal, const std::vector<Constraint>& constraints,
                     int cost, std::vector<int>& walk, std::vector<std::set<int>>& cells_used) {
    const int timestep = static_cast<int>(walk.size()) - 1;
    if (timestep == cost) {
        if (walk.back() == goal) {
            for (int t = 0; t <= cost; ++t) {
                cells_used[static_cast<std::size_t>(t)].insert(walk[static_cast<std::size_t>(t)]);
            }
        }
        return;
    }
    const int cell = walk.back();
    const mapf::Neighbors neighbors = grid.neighbors(cell);
    std::vector<int> candidates(neighbors.begin(), neighbors.end());
    candidates.push_back(cell);
    for (const int next : candidates) {
        if (move_allowed(constraints, cell, next, timestep + 1)) {
            walk.push_back(next);
            enumerate_walks(grid, goal, constraints, cost, walk, cells_used);
            walk.pop_back();
        }
    }
}

// Expected MDD layers by exhaustive enumeration; empty when no walk exists.
Layers brute_force_layers(const Grid& grid, int start, int goal,
                          const std::vector<Constraint>& constraints, int cost) {
    // The agent rests on the goal from `cost` on, so later goal constraints
    // rule the cost out, and so does a constraint on the start at time 0.
    for (const Constraint& constraint : constraints) {
        if (constraint.type == ConstraintType::Vertex) {
            if (constraint.to == goal && constraint.timestep >= cost) {
                return {};
            }
            if (constraint.to == start && constraint.timestep == 0) {
                return {};
            }
        }
    }
    std::vector<std::set<int>> cells_used(static_cast<std::size_t>(cost) + 1);
    std::vector<int> walk = {start};
    enumerate_walks(grid, goal, constraints, cost, walk, cells_used);
    if (cells_used[0].empty()) {
        return {};
    }
    Layers layers;
    for (const std::set<int>& cells : cells_used) {
        layers.emplace_back(cells.begin(), cells.end());
    }
    return layers;
}

}  // namespace

TEST_CASE("a corridor has a single optimal path, so every layer has width one") {
    const Grid grid = mapf::grid_from_rows({"..."});

    const Mdd mdd = mdd_of(grid, 0, 2, {}, 2);

    CHECK(mdd.layers == Layers{{0}, {1}, {2}});
    CHECK(mdd.cost() == 2);
    CHECK(mdd.width(1) == 1);
}

TEST_CASE("an open square offers two optimal paths") {
    const Grid grid = mapf::grid_from_rows({"..", ".."});

    const Mdd mdd = mdd_of(grid, 0, 3, {}, 2);

    CHECK(mdd.layers == Layers{{0}, {1, 2}, {3}});
    CHECK(mdd.width(1) == 2);
}

TEST_CASE("a vertex constraint removes a branch") {
    const Grid grid = mapf::grid_from_rows({"..", ".."});

    const Mdd mdd = mdd_of(grid, 0, 3, {vertex(1, 1)}, 2);

    CHECK(mdd.layers == Layers{{0}, {2}, {3}});
}

TEST_CASE("an edge constraint removes a branch") {
    const Grid grid = mapf::grid_from_rows({"..", ".."});

    const Mdd mdd = mdd_of(grid, 0, 3, {edge(2, 3, 2)}, 2);

    CHECK(mdd.layers == Layers{{0}, {1}, {3}});
}

TEST_CASE("cells that only lead into a blocked move are pruned") {
    // Both middle cells are reachable at time 1, but the move out of cell 1
    // is forbidden, so only the walk through cell 3 survives.
    const Grid grid = mapf::grid_from_rows({"...", "...", "..."});
    const int start = grid.cell(0, 0);
    const int goal = grid.cell(1, 1);

    const Mdd mdd = mdd_of(grid, start, goal, {edge(1, 4, 2)}, 2);

    CHECK(mdd.layers == Layers{{0}, {3}, {4}});
}

TEST_CASE("an agent that starts on its goal has a one-layer MDD at cost zero") {
    const Grid grid = mapf::grid_from_rows({"..."});

    const Mdd mdd = mdd_of(grid, 1, 1, {}, 0);

    CHECK(mdd.layers == Layers{{1}});
    CHECK(mdd.cost() == 0);
}

TEST_CASE("a cost above the optimum also holds walks that pause on the goal") {
    const Grid grid = mapf::grid_from_rows({"..."});

    const Mdd mdd = mdd_of(grid, 0, 2, {}, 3);

    CHECK(mdd.layers == Layers{{0}, {0, 1}, {1, 2}, {2}});
}

TEST_CASE("the MDD is empty when no walk of the requested cost exists") {
    const Grid corridor = mapf::grid_from_rows({"..."});
    const Grid walled = mapf::grid_from_rows({".@."});

    SUBCASE("cost below the shortest distance") {
        CHECK(mdd_of(corridor, 0, 2, {}, 1).empty());
    }
    SUBCASE("goal unreachable on the static grid") {
        CHECK(mdd_of(walled, 0, 2, {}, 5).empty());
    }
    SUBCASE("cost below the goal hold time") {
        CHECK(mdd_of(corridor, 0, 2, {vertex(2, 4)}, 3).empty());
        CHECK_FALSE(mdd_of(corridor, 0, 2, {vertex(2, 4)}, 5).empty());
    }
    SUBCASE("constraints block every walk") {
        CHECK(mdd_of(corridor, 0, 2, {vertex(1, 1)}, 2).empty());
    }
    SUBCASE("start forbidden at time zero") {
        CHECK(mdd_of(corridor, 0, 2, {vertex(0, 0)}, 2).empty());
    }
}

TEST_CASE("build_mdd rejects inconsistent arguments") {
    const Grid grid = mapf::grid_from_rows({"..@.."});
    const HeuristicTable heuristic(grid, 1);
    const ConstraintTable table(kAgent, 1);
    const ConstraintTable other_goal(kAgent, 0);

    CHECK_THROWS_AS(mapf::build_mdd(grid, heuristic, 0, table, -1), std::invalid_argument);
    CHECK_THROWS_AS(mapf::build_mdd(grid, heuristic, 2, table, 3), std::invalid_argument);
    CHECK_THROWS_AS(mapf::build_mdd(grid, heuristic, 9, table, 3), std::invalid_argument);
    CHECK_THROWS_AS(mapf::build_mdd(grid, heuristic, 0, other_goal, 3), std::invalid_argument);
}

// Seeded cross-check against exhaustive enumeration of all walks.
TEST_CASE("build_mdd matches exhaustive enumeration on random constrained instances") {
    std::mt19937 rng(20261112);
    const auto pick = [&rng](int bound) {
        return static_cast<int>(rng() % static_cast<std::uint32_t>(bound));
    };

    int non_empty = 0;
    int empty = 0;
    int above_optimum = 0;

    for (int trial = 0; trial < 400; ++trial) {
        const int height = 1 + pick(3);
        const int width = 1 + pick(4);
        std::vector<std::string> rows(static_cast<std::size_t>(height));
        std::vector<int> free_cells;
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                const bool obstacle = pick(6) == 0;
                rows[static_cast<std::size_t>(r)].push_back(obstacle ? '@' : '.');
                if (!obstacle) {
                    free_cells.push_back(r * width + c);
                }
            }
        }
        if (free_cells.empty()) {
            continue;
        }
        const Grid grid = mapf::grid_from_rows(rows);
        const auto free_cell = [&]() {
            return free_cells[static_cast<std::size_t>(pick(static_cast<int>(free_cells.size())))];
        };
        const int start = free_cell();
        const int goal = free_cell();

        std::vector<Constraint> constraints;
        const int constraint_count = pick(5);
        for (int i = 0; i < constraint_count; ++i) {
            const int from = free_cell();
            const mapf::Neighbors neighbors = grid.neighbors(from);
            if (pick(3) == 0 && neighbors.count > 0) {
                const int to = neighbors.cells[static_cast<std::size_t>(pick(neighbors.count))];
                constraints.push_back(edge(from, to, 1 + pick(5)));
            } else {
                constraints.push_back(vertex(pick(3) == 0 ? goal : from, pick(6)));
            }
        }

        // Use the agent's optimal cost when it has one, plus a random slack,
        // so that both exact and looser MDDs are checked.
        const HeuristicTable heuristic(grid, goal);
        const ConstraintTable table = table_of(goal, constraints);
        const mapf::SearchResult optimal = mapf::find_path(
            grid, heuristic, start, table,
            std::chrono::steady_clock::now() + std::chrono::seconds(30));
        const int slack = pick(3);
        const int cost =
            optimal.status == mapf::SearchStatus::Solved ? optimal.cost + slack : pick(6);
        if (cost > 7) {
            continue;  // keeps the exhaustive enumeration small
        }

        const Mdd mdd = mapf::build_mdd(grid, heuristic, start, table, cost);
        const Layers expected = brute_force_layers(grid, start, goal, constraints, cost);

        CAPTURE(trial);
        CAPTURE(cost);
        CHECK(mdd.layers == expected);
        if (optimal.status == mapf::SearchStatus::Solved && slack == 0) {
            CHECK_FALSE(mdd.empty());
        }
        (expected.empty() ? empty : non_empty) += 1;
        if (!expected.empty() && slack > 0) {
            ++above_optimum;
        }
    }

    MESSAGE("random MDD instances: non-empty=" << non_empty << " empty=" << empty
                                               << " above optimum=" << above_optimum);
    CHECK(non_empty >= 100);
    CHECK(empty >= 20);
    CHECK(above_optimum >= 30);
}
