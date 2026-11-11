#include <doctest.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/instance.h"

using mapf::Agent;
using mapf::Grid;

namespace {

const std::string kDataDir = MAPF_TEST_DATA_DIR;

Grid map_from_text(const std::string& text) {
    std::istringstream in(text);
    return mapf::parse_map(in, "test.map");
}

std::vector<Agent> scenario_from_text(const std::string& text, const Grid& grid, int num_agents) {
    std::istringstream in(text);
    return mapf::parse_scenario(in, "test.scen", grid, num_agents);
}

// A 3-row, 4-column map with one obstacle at (x=1, y=1).
Grid small_map() {
    return mapf::grid_from_rows({"....", ".@..", "...."});
}

std::string scenario_row(int start_x, int start_y, int goal_x, int goal_y) {
    return "0\ttest.map\t4\t3\t" + std::to_string(start_x) + "\t" + std::to_string(start_y) +
           "\t" + std::to_string(goal_x) + "\t" + std::to_string(goal_y) + "\t1.00000000\n";
}

}  // namespace

TEST_CASE("parse_map reads the MovingAI header and rows") {
    const Grid grid = map_from_text("type octile\nheight 2\nwidth 3\nmap\n.@.\n...\n");

    CHECK(grid.height() == 2);
    CHECK(grid.width() == 3);
    CHECK(grid.passable_count() == 5);
    CHECK_FALSE(grid.passable(grid.cell(0, 1)));
}

TEST_CASE("parse_map accepts Windows line endings and a missing final newline") {
    const Grid grid = map_from_text("type octile\r\nheight 2\r\nwidth 2\r\nmap\r\n.T\r\n..");

    CHECK(grid.height() == 2);
    CHECK(grid.width() == 2);
    CHECK(grid.passable_count() == 3);
}

TEST_CASE("parse_map accepts width before height") {
    const Grid grid = map_from_text("type octile\nwidth 3\nheight 1\nmap\n...\n");

    CHECK(grid.height() == 1);
    CHECK(grid.width() == 3);
}

TEST_CASE("parse_map rejects malformed files with the line number") {
    CHECK_THROWS_WITH_AS(map_from_text("type octile\nheight 2\nwidth 3\n"),
                         doctest::Contains("missing 'map' line"), std::runtime_error);
    CHECK_THROWS_WITH_AS(map_from_text("type octile\nheight 2\nmap\n...\n...\n"),
                         doctest::Contains("height and width"), std::runtime_error);
    CHECK_THROWS_WITH_AS(map_from_text("type octile\nheight zero\nwidth 3\nmap\n"),
                         doctest::Contains("test.map:2"), std::runtime_error);
    CHECK_THROWS_WITH_AS(map_from_text("type octile\nheight 2\nwidth 3\nmap\n...\n"),
                         doctest::Contains("expected 2 map rows, found 1"), std::runtime_error);
    CHECK_THROWS_WITH_AS(map_from_text("type octile\nheight 2\nwidth 3\nmap\n...\n..\n"),
                         doctest::Contains("test.map:6"), std::runtime_error);
    CHECK_THROWS_WITH_AS(map_from_text("type octile\nheight 1\nwidth 3\nmap\n.?.\n"),
                         doctest::Contains("unknown terrain character"), std::runtime_error);
    CHECK_THROWS_WITH_AS(map_from_text("size 3\nheight 1\nwidth 3\nmap\n...\n"),
                         doctest::Contains("unexpected header line"), std::runtime_error);
}

TEST_CASE("parse_scenario maps (x, y) to (col, row)") {
    const Grid grid = small_map();
    const std::vector<Agent> agents =
        scenario_from_text("version 1\n" + scenario_row(3, 0, 0, 2), grid, 1);

    REQUIRE(agents.size() == 1);
    CHECK(agents[0].start == grid.cell(0, 3));
    CHECK(agents[0].goal == grid.cell(2, 0));
}

TEST_CASE("parse_scenario takes the first k rows") {
    const Grid grid = small_map();
    const std::string text = "version 1\n" + scenario_row(0, 0, 3, 2) + scenario_row(2, 1, 0, 1) +
                             scenario_row(3, 0, 0, 2);

    const std::vector<Agent> two = scenario_from_text(text, grid, 2);
    REQUIRE(two.size() == 2);
    CHECK(two[0].start == grid.cell(0, 0));
    CHECK(two[1].start == grid.cell(1, 2));

    CHECK(scenario_from_text(text, grid, 3).size() == 3);
}

TEST_CASE("parse_scenario allows an agent that starts on its goal") {
    const Grid grid = small_map();
    const std::vector<Agent> agents = scenario_from_text(scenario_row(2, 2, 2, 2), grid, 1);

    CHECK(agents[0].start == agents[0].goal);
}

TEST_CASE("parse_scenario rejects invalid requests and rows") {
    const Grid grid = small_map();
    const std::string one_row = "version 1\n" + scenario_row(0, 0, 3, 2);

    CHECK_THROWS_AS(scenario_from_text(one_row, grid, 0), std::runtime_error);
    CHECK_THROWS_WITH_AS(scenario_from_text(one_row, grid, 2),
                         doctest::Contains("requested 2 agents, but the scenario has only 1"),
                         std::runtime_error);
    CHECK_THROWS_WITH_AS(scenario_from_text(scenario_row(1, 1, 3, 2), grid, 1),
                         doctest::Contains("start (1, 1) is on an obstacle"), std::runtime_error);
    CHECK_THROWS_WITH_AS(scenario_from_text(scenario_row(0, 0, 1, 1), grid, 1),
                         doctest::Contains("goal (1, 1) is on an obstacle"), std::runtime_error);
    CHECK_THROWS_WITH_AS(scenario_from_text(scenario_row(4, 0, 0, 0), grid, 1),
                         doctest::Contains("start (4, 0) is outside the map"), std::runtime_error);
    CHECK_THROWS_WITH_AS(scenario_from_text(scenario_row(0, 0, 0, 3), grid, 1),
                         doctest::Contains("goal (0, 3) is outside the map"), std::runtime_error);
    CHECK_THROWS_WITH_AS(scenario_from_text("0\ttest.map\t4\t3\t0\t0\n", grid, 1),
                         doctest::Contains("test.scen:1"), std::runtime_error);
    CHECK_THROWS_WITH_AS(
        scenario_from_text("0\ttest.map\t8\t8\t0\t0\t3\t2\t1.0\n", grid, 1),
        doctest::Contains("scenario is for a 8x8 map, but the map is 4x3"), std::runtime_error);
}

TEST_CASE("parse_scenario rejects agents that share a start or a goal") {
    const Grid grid = small_map();

    CHECK_THROWS_WITH_AS(
        scenario_from_text(scenario_row(0, 0, 3, 2) + scenario_row(0, 0, 3, 0), grid, 2),
        doctest::Contains("start cell is already used"), std::runtime_error);
    CHECK_THROWS_WITH_AS(
        scenario_from_text(scenario_row(0, 0, 3, 2) + scenario_row(2, 0, 3, 2), grid, 2),
        doctest::Contains("goal cell is already used"), std::runtime_error);
    // One agent's goal may be another agent's start.
    CHECK_NOTHROW(scenario_from_text(scenario_row(0, 0, 3, 2) + scenario_row(3, 2, 0, 0), grid, 2));
}

TEST_CASE("load_instance reads the hand-written files in tests/data") {
    const mapf::Instance instance =
        mapf::load_instance(kDataDir + "/tiny.map", kDataDir + "/tiny.scen", 3);

    CHECK(instance.grid.height() == 4);
    CHECK(instance.grid.width() == 5);
    CHECK(instance.grid.passable_count() == 16);

    REQUIRE(instance.agents.size() == 3);
    CHECK(instance.agents[0].start == 0);
    CHECK(instance.agents[0].goal == 4);
    CHECK(instance.agents[1].start == 8);
    CHECK(instance.agents[1].goal == 15);
    CHECK(instance.agents[2].start == 11);
    CHECK(instance.agents[2].goal == 18);
}

// Expected values were counted from the benchmark files with shell tools,
// independently of the loader: 819 '.' cells, 204 '@' and one 'T'; 409 agent
// rows; cells computed as y * 32 + x from the scenario columns.
TEST_CASE("load_instance reads an unmodified MovingAI benchmark map and scenario") {
    const std::string map_path = kDataDir + "/movingai/random-32-32-20.map";
    const std::string scen_path = kDataDir + "/movingai/random-32-32-20-random-1.scen";

    const mapf::Instance instance = mapf::load_instance(map_path, scen_path, 409);

    CHECK(instance.grid.height() == 32);
    CHECK(instance.grid.width() == 32);
    CHECK(instance.grid.passable_count() == 819);

    REQUIRE(instance.agents.size() == 409);
    CHECK(instance.agents[0].start == 517);
    CHECK(instance.agents[0].goal == 799);
    CHECK(instance.agents[1].start == 949);
    CHECK(instance.agents[1].goal == 728);
    CHECK(instance.agents[9].start == 235);
    CHECK(instance.agents[9].goal == 96);
    CHECK(instance.agents[408].start == 110);
    CHECK(instance.agents[408].goal == 592);

    CHECK(mapf::load_instance(map_path, scen_path, 10).agents.size() == 10);
    CHECK_THROWS_WITH_AS(mapf::load_instance(map_path, scen_path, 410),
                         doctest::Contains("requested 410 agents, but the scenario has only 409"),
                         std::runtime_error);
}

TEST_CASE("load_instance reports missing files") {
    CHECK_THROWS_WITH_AS(mapf::load_instance(kDataDir + "/no_such.map", kDataDir + "/tiny.scen", 1),
                         doctest::Contains("cannot open map file"), std::runtime_error);
    CHECK_THROWS_WITH_AS(mapf::load_instance(kDataDir + "/tiny.map", kDataDir + "/no_such.scen", 1),
                         doctest::Contains("cannot open scenario file"), std::runtime_error);
}
