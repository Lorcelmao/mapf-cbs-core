#include "app/cli_options.h"

#include <cmath>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace mapf {
namespace {

bool is_known_option(const std::string& name) {
    return name == "--map" || name == "--scen" || name == "--agents" || name == "--solver" ||
           name == "--w" || name == "--conflict" || name == "--seed" || name == "--timeout" ||
           name == "--output" || name == "--paths";
}

bool looks_like_option(const std::string& value) {
    return value.size() >= 2 && value[0] == '-' && value[1] == '-';
}

int parse_int_value(const std::string& name, const std::string& value) {
    std::size_t pos = 0;
    int parsed = 0;
    try {
        parsed = std::stoi(value, &pos);
    } catch (const std::exception&) {
        throw std::invalid_argument("option " + name + " needs an integer, got '" + value + "'");
    }
    if (pos != value.size()) {
        throw std::invalid_argument("option " + name + " needs an integer, got '" + value + "'");
    }
    return parsed;
}

double parse_double_value(const std::string& name, const std::string& value) {
    std::size_t pos = 0;
    double parsed = 0.0;
    try {
        parsed = std::stod(value, &pos);
    } catch (const std::exception&) {
        throw std::invalid_argument("option " + name + " needs a number, got '" + value + "'");
    }
    if (pos != value.size() || !std::isfinite(parsed)) {
        throw std::invalid_argument("option " + name + " needs a number, got '" + value + "'");
    }
    return parsed;
}

}  // namespace

CliOptions parse_cli(const std::vector<std::string>& args) {
    for (const std::string& arg : args) {
        if (arg == "--help" || arg == "-h") {
            CliOptions help_options;
            help_options.help = true;
            return help_options;
        }
    }

    CliOptions opts;
    std::set<std::string> seen;
    bool have_agents = false;

    std::size_t i = 0;
    while (i < args.size()) {
        const std::string& name = args[i];
        if (!is_known_option(name)) {
            throw std::invalid_argument("unknown option '" + name + "'");
        }
        if (i + 1 >= args.size() || looks_like_option(args[i + 1])) {
            throw std::invalid_argument("option " + name + " needs a value");
        }
        const std::string& value = args[i + 1];
        if (!seen.insert(name).second) {
            throw std::invalid_argument("option " + name + " given more than once");
        }

        if (name == "--map") {
            if (value.empty()) {
                throw std::invalid_argument("option --map needs a value");
            }
            opts.map_path = value;
        } else if (name == "--scen") {
            if (value.empty()) {
                throw std::invalid_argument("option --scen needs a value");
            }
            opts.scen_path = value;
        } else if (name == "--agents") {
            const int parsed = parse_int_value(name, value);
            if (parsed < 1) {
                throw std::invalid_argument("option --agents must be at least 1");
            }
            opts.agents = parsed;
            have_agents = true;
        } else if (name == "--solver") {
            if (value != "cbs" && value != "ecbs") {
                throw std::invalid_argument("option --solver must be cbs or ecbs, got '" + value + "'");
            }
            opts.solver = value;
        } else if (name == "--w") {
            const double parsed = parse_double_value(name, value);
            if (parsed < 1.0) {
                throw std::invalid_argument("option --w must be at least 1");
            }
            opts.w = parsed;
        } else if (name == "--conflict") {
            if (value != "first" && value != "earliest" && value != "cardinal") {
                throw std::invalid_argument(
                    "option --conflict must be first, earliest or cardinal, got '" + value + "'");
            }
            opts.conflict_policy = value;
        } else if (name == "--seed") {
            const int parsed = parse_int_value(name, value);
            if (parsed < 0) {
                throw std::invalid_argument("option --seed must not be negative");
            }
            opts.seed = parsed;
        } else if (name == "--timeout") {
            const double parsed = parse_double_value(name, value);
            if (parsed <= 0.0) {
                throw std::invalid_argument("option --timeout must be positive");
            }
            opts.timeout_s = parsed;
        } else if (name == "--output") {
            if (value.empty()) {
                throw std::invalid_argument("option --output needs a value");
            }
            opts.output_path = value;
        } else {  // --paths
            if (value.empty()) {
                throw std::invalid_argument("option --paths needs a value");
            }
            opts.paths_path = value;
        }

        i += 2;
    }

    if (opts.map_path.empty()) {
        throw std::invalid_argument("missing required option --map");
    }
    if (opts.scen_path.empty()) {
        throw std::invalid_argument("missing required option --scen");
    }
    if (!have_agents) {
        throw std::invalid_argument("missing required option --agents");
    }
    if (opts.solver.empty()) {
        throw std::invalid_argument("missing required option --solver");
    }
    if (opts.output_path.empty()) {
        throw std::invalid_argument("missing required option --output");
    }

    return opts;
}

std::string cli_usage() {
    return "Usage: mapf_solver --map <file.map> --scen <file.scen> --agents <k> --solver cbs|ecbs "
           "--output <result.json> [options]\n"
           "\n"
           "Required options:\n"
           "  --map <file.map>        map file describing the grid\n"
           "  --scen <file.scen>      scenario file with start and goal locations\n"
           "  --agents <k>            number of agents to plan for, k >= 1\n"
           "  --solver cbs|ecbs       conflict-based search variant\n"
           "  --output <result.json>  file to write the solution statistics to\n"
           "\n"
           "Optional options:\n"
           "  --w <bound>             suboptimality bound, >= 1 (default 1)\n"
           "  --conflict first|earliest|cardinal\n"
           "                          conflict selection strategy (default first)\n"
           "  --seed <n>              random seed, n >= 0 (default 0)\n"
           "  --timeout <seconds>     solver time limit, > 0 (default 60)\n"
           "  --paths <file>          file to write the per-agent paths to (default: none)\n"
           "  --help, -h              print this usage text and exit\n";
}

}  // namespace mapf
