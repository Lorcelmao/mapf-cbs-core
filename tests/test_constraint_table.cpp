#include <doctest.h>

#include <stdexcept>

#include "core/types.h"
#include "lowlevel/constraint_table.h"

using mapf::Constraint;
using mapf::ConstraintTable;
using mapf::ConstraintType;

namespace {

Constraint vertex(int agent, int cell, int timestep) {
    return Constraint{agent, ConstraintType::Vertex, -1, cell, timestep};
}

Constraint edge(int agent, int from, int to, int timestep) {
    return Constraint{agent, ConstraintType::Edge, from, to, timestep};
}

}  // namespace

TEST_CASE("an empty constraint table blocks nothing") {
    const ConstraintTable table(0, 7);

    CHECK(table.agent() == 0);
    CHECK(table.goal() == 7);
    CHECK(table.latest_time() == -1);
    CHECK(table.goal_hold_time() == 0);
    CHECK_FALSE(table.is_vertex_blocked(7, 0));
    CHECK_FALSE(table.is_edge_blocked(6, 7, 1));
}

TEST_CASE("a vertex constraint blocks one cell at one timestep") {
    ConstraintTable table(0, 7);
    table.add(vertex(0, 3, 4));

    CHECK(table.is_vertex_blocked(3, 4));
    CHECK_FALSE(table.is_vertex_blocked(3, 3));
    CHECK_FALSE(table.is_vertex_blocked(3, 5));
    CHECK_FALSE(table.is_vertex_blocked(4, 4));
    CHECK(table.latest_time() == 4);
}

TEST_CASE("an edge constraint is directed and uses the arrival time") {
    ConstraintTable table(0, 7);
    table.add(edge(0, 2, 3, 5));

    CHECK(table.is_edge_blocked(2, 3, 5));
    CHECK_FALSE(table.is_edge_blocked(3, 2, 5));
    CHECK_FALSE(table.is_edge_blocked(2, 3, 4));
    CHECK_FALSE(table.is_edge_blocked(2, 3, 6));
    // An edge constraint does not forbid standing on either cell.
    CHECK_FALSE(table.is_vertex_blocked(2, 4));
    CHECK_FALSE(table.is_vertex_blocked(3, 5));
    CHECK(table.latest_time() == 5);
}

TEST_CASE("constraints of other agents are ignored") {
    ConstraintTable table(1, 7);
    table.add(vertex(0, 3, 4));
    table.add(edge(2, 2, 3, 5));

    CHECK_FALSE(table.is_vertex_blocked(3, 4));
    CHECK_FALSE(table.is_edge_blocked(2, 3, 5));
    CHECK(table.latest_time() == -1);
}

TEST_CASE("latest_time is the largest timestep over vertex and edge constraints") {
    ConstraintTable table(0, 7);
    table.add(vertex(0, 3, 4));
    table.add(edge(0, 2, 3, 9));
    table.add(vertex(0, 5, 6));

    CHECK(table.latest_time() == 9);
}

TEST_CASE("goal_hold_time is one more than the latest vertex constraint on the goal") {
    ConstraintTable table(0, 7);
    CHECK(table.goal_hold_time() == 0);

    table.add(vertex(0, 3, 10));
    CHECK(table.goal_hold_time() == 0);

    table.add(vertex(0, 7, 4));
    CHECK(table.goal_hold_time() == 5);

    table.add(vertex(0, 7, 2));
    CHECK(table.goal_hold_time() == 5);

    table.add(vertex(0, 7, 0));
    CHECK(table.goal_hold_time() == 5);

    // An edge into the goal does not stop the agent from resting there.
    table.add(edge(0, 6, 7, 12));
    CHECK(table.goal_hold_time() == 5);
}

TEST_CASE("adding the same constraint twice changes nothing") {
    ConstraintTable table(0, 7);
    table.add(vertex(0, 3, 4));
    table.add(vertex(0, 3, 4));

    CHECK(table.is_vertex_blocked(3, 4));
    CHECK(table.latest_time() == 4);
}

TEST_CASE("malformed constraints are rejected") {
    ConstraintTable table(0, 7);

    CHECK_THROWS_AS(table.add(vertex(0, 3, -1)), std::invalid_argument);
    CHECK_THROWS_AS(table.add(edge(0, 2, 3, -1)), std::invalid_argument);
    CHECK_THROWS_AS(table.add(edge(0, 2, 3, 0)), std::invalid_argument);
    CHECK_NOTHROW(table.add(vertex(0, 3, 0)));
    CHECK(table.is_vertex_blocked(3, 0));
}
