// Micro-benchmark of the low-level search on one MovingAI instance.
//
// For every agent it times find_path in four situations that a CBS run
// produces: no constraints (the root node), a few and many vertex constraints
// placed on the agent's own shortest path (deeper constraint-tree nodes), and a
// late constraint on the goal (the agent must keep moving for a long time).
// It reports the average time and the number of states per call.
//
// Usage: mapf_bench_lowlevel <file.map> <file.scen> <agents>

#include <chrono>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "core/instance.h"
#include "core/types.h"
#include "lowlevel/constraint_table.h"
#include "lowlevel/heuristic_table.h"
#include "lowlevel/space_time_astar.h"

namespace {

using Clock = std::chrono::steady_clock;

struct Totals {
    std::uint64_t calls = 0;
    std::uint64_t expanded = 0;
    std::uint64_t generated = 0;
    std::uint64_t cost = 0;
    double seconds = 0.0;
};

// Forbids the agent's own shortest path at `count` evenly spread timesteps.
mapf::ConstraintTable path_constraints(int agent, int goal, const mapf::Path& path, int count) {
    mapf::ConstraintTable table(agent, goal);
    const int last = static_cast<int>(path.size()) - 1;
    for (int i = 1; i <= count && last >= 1; ++i) {
        const int timestep = 1 + (i * (last - 1)) / (count + 1);
        table.add(mapf::Constraint{agent, mapf::ConstraintType::Vertex, -1,
                                   path[static_cast<std::size_t>(timestep)], timestep});
    }
    return table;
}

void run(const std::string& label, const mapf::Instance& instance,
         const std::vector<mapf::HeuristicTable>& heuristics,
         const std::vector<mapf::ConstraintTable>& tables, int repeats) {
    Totals totals;
    const mapf::Deadline deadline = Clock::now() + std::chrono::hours(1);
    for (int repeat = 0; repeat < repeats; ++repeat) {
        for (std::size_t agent = 0; agent < instance.agents.size(); ++agent) {
            const auto started = Clock::now();
            const mapf::SearchResult result = mapf::find_path(
                instance.grid, heuristics[agent], instance.agents[agent].start, tables[agent],
                deadline);
            totals.seconds += std::chrono::duration<double>(Clock::now() - started).count();
            if (result.status != mapf::SearchStatus::Solved) {
                continue;
            }
            ++totals.calls;
            totals.expanded += result.expanded;
            totals.generated += result.generated;
            totals.cost += static_cast<std::uint64_t>(result.cost);
        }
    }
    const double calls = static_cast<double>(totals.calls);
    std::cout << std::left << std::setw(34) << label << std::right << std::fixed
              << std::setw(8) << totals.calls << std::setw(12) << std::setprecision(2)
              << (totals.seconds / calls) * 1e6 << std::setw(12) << std::setprecision(1)
              << static_cast<double>(totals.cost) / calls << std::setw(12)
              << static_cast<double>(totals.expanded) / calls << std::setw(12)
              << static_cast<double>(totals.generated) / calls << '\n';
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: mapf_bench_lowlevel <file.map> <file.scen> <agents>\n";
        return 2;
    }
    try {
        const mapf::Instance instance = mapf::load_instance(argv[1], argv[2], std::stoi(argv[3]));
        const std::size_t agent_count = instance.agents.size();

        const auto heuristics_started = Clock::now();
        std::vector<mapf::HeuristicTable> heuristics;
        heuristics.reserve(agent_count);
        for (const mapf::Agent& agent : instance.agents) {
            heuristics.emplace_back(instance.grid, agent.goal);
        }
        const double heuristic_seconds =
            std::chrono::duration<double>(Clock::now() - heuristics_started).count();

        std::vector<mapf::ConstraintTable> none;
        std::vector<mapf::ConstraintTable> few;
        std::vector<mapf::ConstraintTable> many;
        std::vector<mapf::ConstraintTable> late_goal;
        const mapf::Deadline deadline = Clock::now() + std::chrono::hours(1);
        for (std::size_t i = 0; i < agent_count; ++i) {
            const int agent = static_cast<int>(i);
            const int goal = instance.agents[i].goal;
            none.emplace_back(agent, goal);
            const mapf::SearchResult shortest = mapf::find_path(
                instance.grid, heuristics[i], instance.agents[i].start, none.back(), deadline);
            few.push_back(path_constraints(agent, goal, shortest.path, 3));
            many.push_back(path_constraints(agent, goal, shortest.path, 15));
            late_goal.emplace_back(agent, goal);
            late_goal.back().add(mapf::Constraint{agent, mapf::ConstraintType::Vertex, -1, goal,
                                                  2 * shortest.cost + 10});
        }

        std::cout << "map " << argv[1] << ": " << instance.grid.height() << "x"
                  << instance.grid.width() << ", " << instance.grid.passable_count()
                  << " passable cells, " << agent_count << " agents\n";
        std::cout << "heuristic tables: " << std::fixed << std::setprecision(2)
                  << (heuristic_seconds / static_cast<double>(agent_count)) * 1e6
                  << " us per agent\n\n";
        std::cout << std::left << std::setw(34) << "scenario" << std::right << std::setw(8)
                  << "calls" << std::setw(12) << "us/call" << std::setw(12) << "cost"
                  << std::setw(12) << "expanded" << std::setw(12) << "generated" << '\n';

        const int repeats = 20;
        run("no constraints", instance, heuristics, none, repeats);
        run("3 constraints on own path", instance, heuristics, few, repeats);
        run("15 constraints on own path", instance, heuristics, many, repeats);
        run("goal blocked at 2*cost+10", instance, heuristics, late_goal, repeats);
    } catch (const std::exception& error) {
        std::cerr << "mapf_bench_lowlevel: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
