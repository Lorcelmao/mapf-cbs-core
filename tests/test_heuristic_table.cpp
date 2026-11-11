#include <doctest.h>

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/grid.h"
#include "lowlevel/heuristic_table.h"

TEST_CASE("An empty grid gives Manhattan distances to the goal") {
    const mapf::Grid grid = mapf::grid_from_rows({
        ".....",
        ".....",
        ".....",
        ".....",
    });
    const int goal_row = 2;
    const int goal_col = 3;
    const int goal = grid.cell(goal_row, goal_col);
    const mapf::HeuristicTable table(grid, goal);

    for (int cell = 0; cell < grid.size(); ++cell) {
        const int expected = std::abs(grid.row(cell) - goal_row) +
                             std::abs(grid.col(cell) - goal_col);
        CHECK(table.distance(cell) == expected);
        CHECK(table.reachable(cell));
    }
}

TEST_CASE("The goal cell has distance zero and is reachable") {
    const mapf::Grid grid = mapf::grid_from_rows({
        "...",
        "...",
    });
    const int goal = grid.cell(1, 1);
    const mapf::HeuristicTable table(grid, goal);

    CHECK(table.goal() == goal);
    CHECK(table.distance(goal) == 0);
    CHECK(table.reachable(goal));
}

TEST_CASE("A wall forces a detour around the blocked column") {
    const mapf::Grid grid = mapf::grid_from_rows({
        "G.@..",
        "..@..",
        ".....",
    });
    const int goal = grid.cell(0, 0);
    const mapf::HeuristicTable table(grid, goal);

    CHECK(table.distance(grid.cell(0, 0)) == 0);
    CHECK(table.distance(grid.cell(0, 1)) == 1);
    CHECK(table.distance(grid.cell(1, 0)) == 1);
    CHECK(table.distance(grid.cell(1, 1)) == 2);
    CHECK(table.distance(grid.cell(2, 0)) == 2);
    CHECK(table.distance(grid.cell(2, 1)) == 3);
    CHECK(table.distance(grid.cell(2, 2)) == 4);
    CHECK(table.distance(grid.cell(2, 3)) == 5);
    CHECK(table.distance(grid.cell(2, 4)) == 6);
    CHECK(table.distance(grid.cell(1, 3)) == 6);
    CHECK(table.distance(grid.cell(1, 4)) == 7);
    CHECK(table.distance(grid.cell(0, 3)) == 7);
    CHECK(table.distance(grid.cell(0, 4)) == 8);
}

TEST_CASE("Obstacles report kUnreachable and are not reachable") {
    const mapf::Grid grid = mapf::grid_from_rows({
        "...",
        ".@.",
        "...",
    });
    const int obstacle = grid.cell(1, 1);
    const mapf::HeuristicTable table(grid, grid.cell(0, 0));

    CHECK(table.distance(obstacle) == mapf::HeuristicTable::kUnreachable);
    CHECK_FALSE(table.reachable(obstacle));
    CHECK(table.distance(grid.cell(1, 0)) == 1);
    CHECK(table.reachable(grid.cell(1, 0)));
}

TEST_CASE("A region sealed off by walls stays unreachable") {
    const mapf::Grid grid = mapf::grid_from_rows({
        "..@..",
        "..@..",
        "..@..",
    });
    const mapf::HeuristicTable table(grid, grid.cell(0, 0));

    CHECK(table.distance(grid.cell(0, 3)) == mapf::HeuristicTable::kUnreachable);
    CHECK(table.distance(grid.cell(2, 4)) == mapf::HeuristicTable::kUnreachable);
    CHECK_FALSE(table.reachable(grid.cell(0, 3)));
    CHECK_FALSE(table.reachable(grid.cell(2, 4)));
    CHECK(table.distance(grid.cell(2, 1)) == 3);
    CHECK(table.reachable(grid.cell(2, 1)));
}

TEST_CASE("An out-of-bounds goal throws std::invalid_argument") {
    const mapf::Grid grid = mapf::grid_from_rows({
        "...",
        "...",
        "...",
    });

    CHECK_THROWS_AS(mapf::HeuristicTable(grid, -1), std::invalid_argument);
    CHECK_THROWS_AS(mapf::HeuristicTable(grid, grid.size()), std::invalid_argument);
}

TEST_CASE("A goal placed on an obstacle throws std::invalid_argument") {
    const mapf::Grid grid = mapf::grid_from_rows({
        "...",
        ".@.",
        "...",
    });

    CHECK_THROWS_AS(mapf::HeuristicTable(grid, grid.cell(1, 1)), std::invalid_argument);
}
