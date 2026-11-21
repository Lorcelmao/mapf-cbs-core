#include <exception>
#include <fstream>
#include <iostream>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "app/cli_options.h"
#include "app/peak_memory.h"
#include "app/result_writer.h"
#include "core/instance.h"
#include "core/solve_result.h"
#include "lowlevel/heuristic_table.h"

// mapf_solver: runs one solver on one MovingAI instance and writes one JSON
// result file.
//
// Exit codes: 0 when the result file was written and the input was usable
// (including timeouts and solver errors); 1 when the map or scenario could not
// be loaded (a result file with status "error" is still written); 2 for a
// command-line error; 3 when the result file could not be written.

namespace {

using mapf::CliOptions;
using mapf::Instance;
using mapf::SolveResult;
using mapf::SolveStatus;

SolveResult failure(SolveStatus status, const std::string& reason) {
    SolveResult result;
    result.status = status;
    result.error = reason;
    return result;
}

SolveResult solve(const Instance& instance, const CliOptions& options) {
    // The sum of single-agent shortest paths is a lower bound on the optimal
    // sum-of-costs whichever solver runs, and an agent that cannot reach its
    // goal on the static map makes the instance unsolvable.
    int independent_bound = 0;
    for (std::size_t agent = 0; agent < instance.agents.size(); ++agent) {
        const mapf::HeuristicTable heuristic(instance.grid, instance.agents[agent].goal);
        if (!heuristic.reachable(instance.agents[agent].start)) {
            return failure(SolveStatus::NoSolution,
                           "agent " + std::to_string(agent) + " cannot reach its goal");
        }
        independent_bound += heuristic.distance(instance.agents[agent].start);
    }

    // CBS and ECBS are dispatched here.
    SolveResult result = failure(
        SolveStatus::Error, "solver '" + options.solver + "' is not available in this build");
    result.lb_independent = independent_bound;
    result.lb_certified = independent_bound;
    result.lb_source = "independent";
    return result;
}

}  // namespace

int main(int argc, char** argv) {
    CliOptions options;
    try {
        options = mapf::parse_cli(std::vector<std::string>(argv + 1, argv + argc));
    } catch (const std::invalid_argument& error) {
        std::cerr << "mapf_solver: " << error.what() << "\n\n" << mapf::cli_usage();
        return 2;
    }
    if (options.help) {
        std::cout << mapf::cli_usage();
        return 0;
    }

    mapf::RunRecord record;
    record.options = options;
    int exit_code = 0;
    std::optional<Instance> instance;
    try {
        instance.emplace(
            mapf::load_instance(options.map_path, options.scen_path, options.agents));
    } catch (const std::exception& error) {
        record.result = failure(SolveStatus::Error, error.what());
        exit_code = 1;
    }
    if (instance) {
        try {
            record.result = solve(*instance, options);
            if (record.result.status == SolveStatus::Solved && !options.paths_path.empty()) {
                std::ofstream paths_file(options.paths_path);
                if (!paths_file) {
                    throw std::runtime_error("cannot write paths file '" + options.paths_path +
                                             "'");
                }
                mapf::write_paths(paths_file, instance->grid, record.result.paths);
            }
        } catch (const std::bad_alloc&) {
            record.result = failure(SolveStatus::Error, "out of memory");
        } catch (const std::exception& error) {
            record.result = failure(SolveStatus::Error, error.what());
        }
    }
    record.peak_memory_bytes = mapf::peak_memory_bytes();

    std::ofstream output(options.output_path);
    if (!output) {
        std::cerr << "mapf_solver: cannot write result file '" << options.output_path << "'\n";
        return 3;
    }
    mapf::write_result_json(output, record);
    return exit_code;
}
