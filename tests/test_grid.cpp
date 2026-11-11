#include <doctest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "core/grid.h"

using mapf::Grid;
using mapf::grid_from_rows;

namespace {

std::vector<int> neighbor_list(const Grid& grid, int cell) {
    const mapf::Neighbors neighbors = grid.neighbors(cell);
    return std::vector<int>(neighbors.begin(), neighbors.end());
}

}  // namespace

TEST_CASE("grid_from_rows reads dimensions and terrain") {
    const Grid grid = grid_from_rows({".@.", "..T"});

    CHECK(grid.height() == 2);
    CHECK(grid.width() == 3);
    CHECK(grid.size() == 6);
    CHECK(grid.passable_count() == 4);
    CHECK(grid.passable(grid.cell(0, 0)));
    CHECK_FALSE(grid.passable(grid.cell(0, 1)));
    CHECK(grid.passable(grid.cell(1, 1)));
    CHECK_FALSE(grid.passable(grid.cell(1, 2)));
}

TEST_CASE("a cell is row * width + col") {
    const Grid grid = grid_from_rows({"....", "....", "...."});

    CHECK(grid.cell(0, 0) == 0);
    CHECK(grid.cell(0, 3) == 3);
    CHECK(grid.cell(1, 0) == 4);
    CHECK(grid.cell(2, 3) == 11);
    CHECK(grid.row(11) == 2);
    CHECK(grid.col(11) == 3);
    CHECK(grid.row(4) == 1);
    CHECK(grid.col(4) == 0);
}

TEST_CASE("in_bounds rejects coordinates outside the grid") {
    const Grid grid = grid_from_rows({"...", "..."});

    CHECK(grid.in_bounds(0, 0));
    CHECK(grid.in_bounds(1, 2));
    CHECK_FALSE(grid.in_bounds(-1, 0));
    CHECK_FALSE(grid.in_bounds(0, -1));
    CHECK_FALSE(grid.in_bounds(2, 0));
    CHECK_FALSE(grid.in_bounds(0, 3));
}

TEST_CASE("terrain characters follow the MovingAI convention") {
    for (const char terrain : std::string(".GS")) {
        CAPTURE(terrain);
        CHECK(mapf::is_passable_terrain(terrain));
    }
    for (const char terrain : std::string("@OTW")) {
        CAPTURE(terrain);
        CHECK_FALSE(mapf::is_passable_terrain(terrain));
    }
    CHECK_THROWS_AS(mapf::is_passable_terrain('x'), std::invalid_argument);
    CHECK_THROWS_AS(mapf::is_passable_terrain(' '), std::invalid_argument);
}

TEST_CASE("neighbors are listed up, down, left, right") {
    const Grid grid = grid_from_rows({"...", "...", "..."});

    CHECK(neighbor_list(grid, grid.cell(1, 1)) == std::vector<int>{1, 7, 3, 5});
}

TEST_CASE("neighbors stop at the grid border") {
    const Grid grid = grid_from_rows({"...", "...", "..."});

    CHECK(neighbor_list(grid, grid.cell(0, 0)) == std::vector<int>{3, 1});
    CHECK(neighbor_list(grid, grid.cell(2, 2)) == std::vector<int>{5, 7});
    CHECK(neighbor_list(grid, grid.cell(0, 1)) == std::vector<int>{4, 0, 2});
}

TEST_CASE("neighbors skip obstacles") {
    const Grid grid = grid_from_rows({"..", "@."});

    CHECK(neighbor_list(grid, grid.cell(0, 0)) == std::vector<int>{1});
    CHECK(neighbor_list(grid, grid.cell(1, 1)) == std::vector<int>{1});
}

TEST_CASE("a walled-in cell has no neighbors") {
    const Grid grid = grid_from_rows({"@.@"});

    CHECK(grid.neighbors(grid.cell(0, 1)).count == 0);
}

TEST_CASE("malformed rows are rejected") {
    CHECK_THROWS_AS(grid_from_rows({}), std::invalid_argument);
    CHECK_THROWS_AS(grid_from_rows({""}), std::invalid_argument);
    CHECK_THROWS_AS(grid_from_rows({"...", ".."}), std::invalid_argument);
    CHECK_THROWS_WITH_AS(grid_from_rows({"...", ".x."}), doctest::Contains("row 1, col 1"),
                         std::invalid_argument);
}

TEST_CASE("the Grid constructor checks its sizes") {
    CHECK_THROWS_AS(Grid(0, 3, {}), std::invalid_argument);
    CHECK_THROWS_AS(Grid(2, -1, {}), std::invalid_argument);
    CHECK_THROWS_AS(Grid(2, 2, {1, 1, 1}), std::invalid_argument);
    CHECK_NOTHROW(Grid(2, 2, {1, 0, 1, 1}));
}
