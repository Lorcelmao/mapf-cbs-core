#include "core/instance.h"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mapf {

namespace {

[[noreturn]] void fail(const std::string& source, int line_number, const std::string& message) {
    throw std::runtime_error(source + ":" + std::to_string(line_number) + ": " + message);
}

// Reads one line and drops a trailing carriage return, so files saved with
// Windows line endings parse the same as the original benchmark files.
bool read_line(std::istream& in, std::string& line) {
    if (!std::getline(in, line)) {
        return false;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return true;
}

int scenario_cell(const Grid& grid, int x, int y, const char* role, const std::string& source,
                  int line_number) {
    if (!grid.in_bounds(y, x)) {
        fail(source, line_number,
             std::string(role) + " (" + std::to_string(x) + ", " + std::to_string(y) +
                 ") is outside the map");
    }
    const int cell = grid.cell(y, x);
    if (!grid.passable(cell)) {
        fail(source, line_number,
             std::string(role) + " (" + std::to_string(x) + ", " + std::to_string(y) +
                 ") is on an obstacle");
    }
    return cell;
}

}  // namespace

Grid parse_map(std::istream& in, const std::string& source) {
    int height = -1;
    int width = -1;
    int line_number = 0;
    bool found_map_keyword = false;
    std::string line;

    while (read_line(in, line)) {
        ++line_number;
        std::istringstream fields(line);
        std::string key;
        if (!(fields >> key)) {
            continue;
        }
        if (key == "map") {
            found_map_keyword = true;
            break;
        }
        if (key == "type") {
            continue;
        }
        if (key == "height" || key == "width") {
            int value = 0;
            if (!(fields >> value) || value <= 0) {
                fail(source, line_number, key + " must be a positive integer");
            }
            (key == "height" ? height : width) = value;
            continue;
        }
        fail(source, line_number, "unexpected header line '" + line + "'");
    }
    if (!found_map_keyword) {
        fail(source, line_number, "missing 'map' line");
    }
    if (height < 0 || width < 0) {
        fail(source, line_number, "height and width must come before the 'map' line");
    }

    std::vector<std::string> rows;
    for (int r = 0; r < height; ++r) {
        if (!read_line(in, line)) {
            fail(source, line_number,
                 "expected " + std::to_string(height) + " map rows, found " + std::to_string(r));
        }
        ++line_number;
        if (line.size() != static_cast<std::size_t>(width)) {
            fail(source, line_number,
                 "map row has " + std::to_string(line.size()) + " cells, expected " +
                     std::to_string(width));
        }
        rows.push_back(line);
    }

    try {
        return grid_from_rows(rows);
    } catch (const std::invalid_argument& error) {
        throw std::runtime_error(source + ": " + error.what());
    }
}

std::vector<Agent> parse_scenario(std::istream& in, const std::string& source, const Grid& grid,
                                  int num_agents) {
    if (num_agents <= 0) {
        throw std::runtime_error(source + ": the number of agents must be positive");
    }

    std::vector<Agent> agents;
    std::vector<std::uint8_t> start_taken(static_cast<std::size_t>(grid.size()), 0);
    std::vector<std::uint8_t> goal_taken(static_cast<std::size_t>(grid.size()), 0);
    int line_number = 0;
    std::string line;

    while (static_cast<int>(agents.size()) < num_agents && read_line(in, line)) {
        ++line_number;
        std::istringstream fields(line);
        std::string bucket;
        if (!(fields >> bucket)) {
            continue;
        }
        if (bucket == "version") {
            continue;
        }

        std::string map_name;
        int map_width = 0;
        int map_height = 0;
        int start_x = 0;
        int start_y = 0;
        int goal_x = 0;
        int goal_y = 0;
        double optimal_length = 0.0;
        if (!(fields >> map_name >> map_width >> map_height >> start_x >> start_y >> goal_x >>
              goal_y >> optimal_length)) {
            fail(source, line_number,
                 "expected: bucket map width height start_x start_y goal_x goal_y optimal_length");
        }
        if (map_width != grid.width() || map_height != grid.height()) {
            fail(source, line_number,
                 "scenario is for a " + std::to_string(map_width) + "x" +
                     std::to_string(map_height) + " map, but the map is " +
                     std::to_string(grid.width()) + "x" + std::to_string(grid.height()));
        }

        const int start = scenario_cell(grid, start_x, start_y, "start", source, line_number);
        const int goal = scenario_cell(grid, goal_x, goal_y, "goal", source, line_number);
        if (start_taken[static_cast<std::size_t>(start)] != 0) {
            fail(source, line_number, "start cell is already used by an earlier agent");
        }
        if (goal_taken[static_cast<std::size_t>(goal)] != 0) {
            fail(source, line_number, "goal cell is already used by an earlier agent");
        }
        start_taken[static_cast<std::size_t>(start)] = 1;
        goal_taken[static_cast<std::size_t>(goal)] = 1;
        agents.push_back(Agent{start, goal});
    }

    if (static_cast<int>(agents.size()) < num_agents) {
        throw std::runtime_error(source + ": requested " + std::to_string(num_agents) +
                                 " agents, but the scenario has only " +
                                 std::to_string(agents.size()));
    }
    return agents;
}

Grid load_map(const std::string& map_path) {
    std::ifstream in(map_path);
    if (!in) {
        throw std::runtime_error("cannot open map file '" + map_path + "'");
    }
    return parse_map(in, map_path);
}

Instance load_instance(const std::string& map_path, const std::string& scen_path,
                       int num_agents) {
    Grid grid = load_map(map_path);
    std::ifstream in(scen_path);
    if (!in) {
        throw std::runtime_error("cannot open scenario file '" + scen_path + "'");
    }
    std::vector<Agent> agents = parse_scenario(in, scen_path, grid, num_agents);
    return Instance{std::move(grid), std::move(agents)};
}

}  // namespace mapf
