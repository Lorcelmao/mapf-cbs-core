#include <doctest.h>

#include <chrono>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/grid.h"
#include "core/types.h"
#include "lowlevel/constraint_table.h"
#include "lowlevel/heuristic_table.h"
#include "lowlevel/space_time_astar.h"
#include "reference_search.h"

using mapf::Constraint;
using mapf::ConstraintTable;
using mapf::ConstraintType;
using mapf::Grid;
using mapf::HeuristicTable;
using mapf::Path;
using mapf::SearchResult;
using mapf::SearchStatus;

namespace {

const int kAgent = 0;

Constraint vertex(int cell, int timestep) {
    return Constraint{kAgent, ConstraintType::Vertex, -1, cell, timestep};
}

Constraint edge(int from, int to, int timestep) {
    return Constraint{kAgent, ConstraintType::Edge, from, to, timestep};
}

mapf::Deadline far_deadline() {
    return std::chrono::steady_clock::now() + std::chrono::seconds(30);
}

SearchResult search(const Grid& grid, int start, int goal,
                    const std::vector<Constraint>& constraints) {
    const HeuristicTable heuristic(grid, goal);
    ConstraintTable table(kAgent, goal);
    for (const Constraint& constraint : constraints) {
        table.add(constraint);
    }
    return mapf::find_path(grid, heuristic, start, table, far_deadline());
}

// Checks a path against the model in docs/mapf-spec.md by scanning the raw
// constraint list, without using ConstraintTable. Returns an empty string for
// a valid path, otherwise the first defect found.
std::string path_defect(const Grid& grid, int start, int goal,
                        const std::vector<Constraint>& constraints, const Path& path) {
    if (path.empty()) {
        return "path is empty";
    }
    if (path.front() != start) {
        return "path does not begin at the start";
    }
    if (path.back() != goal) {
        return "path does not end at the goal";
    }
    const int last_time = static_cast<int>(path.size()) - 1;
    if (last_time > 0 && path[path.size() - 2] == goal) {
        return "path is not in normal form: it already rests on the goal one step earlier";
    }
    for (int t = 1; t <= last_time; ++t) {
        const int from = path[static_cast<std::size_t>(t - 1)];
        const int to = path[static_cast<std::size_t>(t)];
        bool legal_move = from == to;
        for (const int neighbor : grid.neighbors(from)) {
            legal_move = legal_move || neighbor == to;
        }
        if (!legal_move) {
            return "illegal move at time " + std::to_string(t);
        }
    }
    for (const Constraint& constraint : constraints) {
        const int t = constraint.timestep;
        if (constraint.type == ConstraintType::Vertex) {
            // After the path ends the agent keeps occupying the goal.
            const int cell_at_t = t <= last_time ? path[static_cast<std::size_t>(t)] : goal;
            if (cell_at_t == constraint.to) {
                return "vertex constraint violated at time " + std::to_string(t);
            }
        } else if (t >= 1 && t <= last_time && path[static_cast<std::size_t>(t - 1)] == constraint.from &&
                   path[static_cast<std::size_t>(t)] == constraint.to) {
            return "edge constraint violated at time " + std::to_string(t);
        }
    }
    return "";
}

}  // namespace

TEST_CASE("constrained_successors lists up, down, left, right, then wait") {
    const Grid grid = mapf::grid_from_rows({"...", "...", "..."});
    const ConstraintTable none(kAgent, 8);

    const mapf::Successors successors = mapf::constrained_successors(grid, none, 4, 1);

    CHECK(std::vector<int>(successors.begin(), successors.end()) ==
          std::vector<int>{1, 7, 3, 5, 4});
}

TEST_CASE("constrained_successors removes exactly the forbidden moves") {
    const Grid grid = mapf::grid_from_rows({"...", "...", "..."});
    ConstraintTable table(kAgent, 8);
    table.add(vertex(1, 2));
    table.add(edge(4, 3, 2));
    table.add(vertex(4, 2));

    const mapf::Successors blocked = mapf::constrained_successors(grid, table, 4, 2);
    CHECK(std::vector<int>(blocked.begin(), blocked.end()) == std::vector<int>{7, 5});

    const mapf::Successors later = mapf::constrained_successors(grid, table, 4, 3);
    CHECK(later.count == 5);
}

TEST_CASE("without constraints the cost is the shortest-path distance") {
    const Grid grid = mapf::grid_from_rows({"......", "......", "......", "......"});
    const int start = grid.cell(0, 0);
    const int goal = grid.cell(3, 5);

    const SearchResult result = search(grid, start, goal, {});

    REQUIRE(result.status == SearchStatus::Solved);
    CHECK(result.cost == 8);
    CHECK(result.lower_bound == 8);
    CHECK(result.path.size() == 9);
    CHECK(path_defect(grid, start, goal, {}, result.path) == "");
    // With an exact heuristic and deepest-first tie-breaking the search walks
    // straight down one optimal path.
    CHECK(result.expanded == 8);
}

TEST_CASE("the search goes around a wall") {
    const Grid grid = mapf::grid_from_rows({"..@..", "..@..", "....."});
    const int start = grid.cell(0, 0);
    const int goal = grid.cell(0, 4);

    const SearchResult result = search(grid, start, goal, {});

    REQUIRE(result.status == SearchStatus::Solved);
    CHECK(result.cost == 8);
    CHECK(path_defect(grid, start, goal, {}, result.path) == "");
}

TEST_CASE("an agent that starts on its goal has cost zero") {
    const Grid grid = mapf::grid_from_rows({"..."});

    const SearchResult result = search(grid, 1, 1, {});

    REQUIRE(result.status == SearchStatus::Solved);
    CHECK(result.cost == 0);
    CHECK(result.path == Path{1});
}

TEST_CASE("a vertex constraint on the shortest path makes the agent wait one step") {
    const Grid grid = mapf::grid_from_rows({"....."});
    const std::vector<Constraint> constraints = {vertex(2, 2)};

    const SearchResult result = search(grid, 0, 4, constraints);

    REQUIRE(result.status == SearchStatus::Solved);
    CHECK(result.cost == 5);
    CHECK(path_defect(grid, 0, 4, constraints, result.path) == "");
}

TEST_CASE("an edge constraint forbids the move at its arrival time only") {
    const Grid grid = mapf::grid_from_rows({"..."});
    const std::vector<Constraint> constraints = {edge(0, 1, 1)};

    const SearchResult result = search(grid, 0, 2, constraints);

    REQUIRE(result.status == SearchStatus::Solved);
    CHECK(result.cost == 3);
    // The only optimal plan waits once and then makes the same move at time 2.
    CHECK(result.path == Path{0, 0, 1, 2});
}

TEST_CASE("an edge constraint in the opposite direction does not delay the agent") {
    const Grid grid = mapf::grid_from_rows({"..."});

    const SearchResult result = search(grid, 0, 2, {edge(1, 0, 1)});

    REQUIRE(result.status == SearchStatus::Solved);
    CHECK(result.cost == 2);
}

TEST_CASE("waiting in front of a goal that is blocked on arrival counts as cost") {
    const Grid grid = mapf::grid_from_rows({"..."});
    const std::vector<Constraint> constraints = {vertex(2, 2)};

    const SearchResult result = search(grid, 0, 2, constraints);

    REQUIRE(result.status == SearchStatus::Solved);
    CHECK(result.cost == 3);
    CHECK(path_defect(grid, 0, 2, constraints, result.path) == "");
}

TEST_CASE("a future vertex constraint on the goal delays the final arrival") {
    const Grid grid = mapf::grid_from_rows({"..."});

    SUBCASE("the cost is one more than the constraint time when that exceeds the distance") {
        const std::vector<Constraint> constraints = {vertex(2, 5)};
        const SearchResult result = search(grid, 0, 2, constraints);

        REQUIRE(result.status == SearchStatus::Solved);
        CHECK(result.cost == 6);
        CHECK(path_defect(grid, 0, 2, constraints, result.path) == "");
    }
    SUBCASE("a goal constraint before the earliest arrival changes nothing") {
        const SearchResult result = search(grid, 0, 2, {vertex(2, 0)});

        REQUIRE(result.status == SearchStatus::Solved);
        CHECK(result.cost == 2);
    }
}

TEST_CASE("the agent may reach its goal, leave it, and come back") {
    // The start is forbidden at times 1 to 3, so the agent must step onto its
    // goal (cell 1) at time 1. The goal is forbidden at time 3, so it has to
    // move on to cell 2 and return at time 4.
    const Grid grid = mapf::grid_from_rows({"...."});
    const std::vector<Constraint> constraints = {vertex(0, 1), vertex(0, 2), vertex(0, 3),
                                                 vertex(1, 3)};

    const SearchResult result = search(grid, 0, 1, constraints);

    REQUIRE(result.status == SearchStatus::Solved);
    CHECK(result.cost == 4);
    REQUIRE(result.path.size() == 5);
    CHECK(result.path[1] == 1);
    CHECK(result.path[3] == 2);
    CHECK(result.path[4] == 1);
    CHECK(path_defect(grid, 0, 1, constraints, result.path) == "");
}

TEST_CASE("a goal behind a wall is reported without expanding any state") {
    const Grid grid = mapf::grid_from_rows({".@."});

    const SearchResult result = search(grid, 0, 2, {});

    CHECK(result.status == SearchStatus::NoPath);
    CHECK(result.expanded == 0);
    CHECK(result.path.empty());
}

TEST_CASE("the search terminates when constraints make the problem infeasible") {
    // The agent is walled in on its goal and may not stand there at time 3.
    const Grid grid = mapf::grid_from_rows({"@.@"});

    const SearchResult result = search(grid, 1, 1, {vertex(1, 3)});

    CHECK(result.status == SearchStatus::NoPath);
    CHECK(result.path.empty());
}

TEST_CASE("a vertex constraint on the start at time zero cannot be satisfied") {
    const Grid grid = mapf::grid_from_rows({"..."});

    CHECK(search(grid, 0, 2, {vertex(0, 0)}).status == SearchStatus::NoPath);
}

TEST_CASE("constraints of another agent do not affect the search") {
    const Grid grid = mapf::grid_from_rows({"....."});
    const Constraint other_agent{kAgent + 1, ConstraintType::Vertex, -1, 2, 2};

    const SearchResult result = search(grid, 0, 4, {other_agent});

    REQUIRE(result.status == SearchStatus::Solved);
    CHECK(result.cost == 4);
}

TEST_CASE("equal inputs give the same path") {
    const Grid grid = mapf::grid_from_rows({".....", ".....", ".....", ".....", "....."});
    const std::vector<Constraint> constraints = {vertex(grid.cell(2, 2), 4),
                                                 edge(grid.cell(0, 0), grid.cell(0, 1), 1)};

    const SearchResult first = search(grid, grid.cell(0, 0), grid.cell(4, 4), constraints);
    const SearchResult second = search(grid, grid.cell(0, 0), grid.cell(4, 4), constraints);

    REQUIRE(first.status == SearchStatus::Solved);
    CHECK(first.path == second.path);
    CHECK(first.expanded == second.expanded);
    CHECK(first.generated == second.generated);
}

TEST_CASE("a deadline in the past returns Timeout") {
    const Grid grid = mapf::grid_from_rows({"....."});
    const HeuristicTable heuristic(grid, 4);
    const ConstraintTable table(kAgent, 4);
    const mapf::Deadline past = std::chrono::steady_clock::now() - std::chrono::seconds(1);

    const SearchResult result = mapf::find_path(grid, heuristic, 0, table, past);

    CHECK(result.status == SearchStatus::Timeout);
    CHECK(result.path.empty());
    CHECK(result.lower_bound == 4);
}

TEST_CASE("find_path rejects inconsistent arguments") {
    const Grid grid = mapf::grid_from_rows({"..@.."});
    const HeuristicTable heuristic(grid, 1);

    const ConstraintTable other_goal(kAgent, 0);
    CHECK_THROWS_AS(mapf::find_path(grid, heuristic, 0, other_goal, far_deadline()),
                    std::invalid_argument);

    const ConstraintTable table(kAgent, 1);
    CHECK_THROWS_AS(mapf::find_path(grid, heuristic, 2, table, far_deadline()),
                    std::invalid_argument);
    CHECK_THROWS_AS(mapf::find_path(grid, heuristic, -1, table, far_deadline()),
                    std::invalid_argument);
    CHECK_THROWS_AS(mapf::find_path(grid, heuristic, 5, table, far_deadline()),
                    std::invalid_argument);
}

// Cross-check against tests/reference_search.h, which explores the
// time-expanded graph layer by layer without merging states and without the
// ConstraintTable. Seeded, so every run uses the same instances.
TEST_CASE("find_path matches the reference search on random constrained instances") {
    std::mt19937 rng(20261111);
    const auto pick = [&rng](int bound) {
        return static_cast<int>(rng() % static_cast<std::uint32_t>(bound));
    };

    int solved = 0;
    int infeasible = 0;
    int delayed = 0;

    for (int trial = 0; trial < 500; ++trial) {
        const int height = 1 + pick(6);
        const int width = 1 + pick(6);
        std::vector<std::string> rows(static_cast<std::size_t>(height));
        std::vector<int> free_cells;
        for (int r = 0; r < height; ++r) {
            for (int c = 0; c < width; ++c) {
                const bool obstacle = pick(5) == 0;
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
        const int constraint_count = pick(9);
        for (int i = 0; i < constraint_count; ++i) {
            const int from = free_cell();
            const mapf::Neighbors neighbors = grid.neighbors(from);
            if (pick(3) == 0 && neighbors.count > 0) {
                const int to = neighbors.cells[static_cast<std::size_t>(pick(neighbors.count))];
                constraints.push_back(edge(from, to, 1 + pick(10)));
            } else {
                // Bias towards the goal so that goal constraints are common.
                constraints.push_back(vertex(pick(3) == 0 ? goal : from, pick(11)));
            }
        }

        const int expected = mapf_test::reference_min_cost(grid, start, goal, constraints);
        const SearchResult result = search(grid, start, goal, constraints);

        CAPTURE(trial);
        if (expected < 0) {
            CHECK(result.status == SearchStatus::NoPath);
            ++infeasible;
            continue;
        }
        REQUIRE(result.status == SearchStatus::Solved);
        CHECK(result.cost == expected);
        CHECK(result.lower_bound == result.cost);
        CHECK(result.path.size() == static_cast<std::size_t>(result.cost) + 1);
        CHECK(path_defect(grid, start, goal, constraints, result.path) == "");
        ++solved;
        if (result.cost > HeuristicTable(grid, goal).distance(start)) {
            ++delayed;
        }
    }

    MESSAGE("random instances: solved=" << solved << " infeasible=" << infeasible
                                        << " delayed by constraints=" << delayed);
    // The generator must keep producing every kind of case.
    CHECK(solved >= 100);
    CHECK(infeasible >= 20);
    CHECK(delayed >= 20);
}
