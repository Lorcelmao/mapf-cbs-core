#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "core/grid.h"

// Loading of MovingAI MAPF benchmark files. Every function throws
// std::runtime_error with the source name and line number on malformed input.

namespace mapf {

struct Agent {
    int start;
    int goal;
};

struct Instance {
    Grid grid;
    std::vector<Agent> agents;
};

// Reads a MovingAI .map stream. `source` names the input in error messages.
Grid parse_map(std::istream& in, const std::string& source);

// Reads the first `num_agents` rows of a MovingAI .scen stream as agents on
// `grid`. Scenario coordinates are (x, y) = (col, row) with the origin at the
// top-left cell. Rejects agents that are out of bounds, on an obstacle, or that
// share a start or a goal with an earlier agent.
std::vector<Agent> parse_scenario(std::istream& in, const std::string& source, const Grid& grid,
                                  int num_agents);

Grid load_map(const std::string& map_path);

Instance load_instance(const std::string& map_path, const std::string& scen_path, int num_agents);

}  // namespace mapf
