#include <doctest.h>

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

#include "app/cli_options.h"

namespace mapf {
namespace {

std::vector<std::string> base_required() {
    return {"--map", "m.map", "--scen", "s.scen", "--solver", "cbs", "--output", "o.json"};
}

std::vector<std::string> required() {
    std::vector<std::string> args = base_required();
    args.push_back("--agents");
    args.push_back("3");
    return args;
}

std::vector<std::string> with_args(std::vector<std::string> args,
                                   std::initializer_list<std::string> extras) {
    args.insert(args.end(), extras.begin(), extras.end());
    return args;
}

}  // namespace

TEST_CASE("a full command line fills every field") {
    const std::vector<std::string> args = {
        "--map", "room.map", "--scen", "room.scen", "--agents", "4", "--solver", "ecbs",
        "--w", "1.5", "--conflict", "cardinal", "--seed", "7", "--timeout", "2.5",
        "--output", "result.json", "--paths", "paths.txt"};

    const CliOptions opts = parse_cli(args);

    CHECK(opts.map_path == "room.map");
    CHECK(opts.scen_path == "room.scen");
    CHECK(opts.agents == 4);
    CHECK(opts.solver == "ecbs");
    CHECK(opts.w == doctest::Approx(1.5));
    CHECK(opts.conflict_policy == "cardinal");
    CHECK(opts.seed == 7);
    CHECK(opts.timeout_s == doctest::Approx(2.5));
    CHECK(opts.output_path == "result.json");
    CHECK(opts.paths_path == "paths.txt");
    CHECK_FALSE(opts.help);
}

TEST_CASE("only the required options leaves every default in place") {
    const CliOptions opts = parse_cli(required());

    CHECK(opts.map_path == "m.map");
    CHECK(opts.scen_path == "s.scen");
    CHECK(opts.agents == 3);
    CHECK(opts.solver == "cbs");
    CHECK(opts.output_path == "o.json");
    CHECK(opts.w == doctest::Approx(1.0));
    CHECK(opts.conflict_policy == "first");
    CHECK(opts.seed == 0);
    CHECK(opts.timeout_s == doctest::Approx(60.0));
    CHECK(opts.paths_path.empty());
    CHECK_FALSE(opts.help);
}

TEST_CASE("option order does not matter") {
    const std::vector<std::string> args = {
        "--output", "out.json", "--paths", "p.txt", "--solver", "ecbs", "--agents", "2",
        "--scen",    "s.scen",   "--map",   "m.map", "--conflict", "earliest"};

    const CliOptions opts = parse_cli(args);

    CHECK(opts.output_path == "out.json");
    CHECK(opts.paths_path == "p.txt");
    CHECK(opts.solver == "ecbs");
    CHECK(opts.agents == 2);
    CHECK(opts.scen_path == "s.scen");
    CHECK(opts.map_path == "m.map");
    CHECK(opts.conflict_policy == "earliest");
}

TEST_CASE("an unknown option or a stray value is rejected") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--bogus", "1"})),
                         doctest::Contains("unknown option '--bogus'"), std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"stray"})),
                         doctest::Contains("unknown option 'stray'"), std::invalid_argument);
}

TEST_CASE("a trailing option without a value is rejected") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--seed"})),
                         doctest::Contains("option --seed needs a value"), std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--timeout"})),
                         doctest::Contains("option --timeout needs a value"), std::invalid_argument);
}

TEST_CASE("an option followed by another option has no value") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--seed", "--w", "2"})),
                         doctest::Contains("option --seed needs a value"), std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--paths", "--seed", "1"})),
                         doctest::Contains("option --paths needs a value"), std::invalid_argument);
}

TEST_CASE("an empty value is rejected for string options") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--paths", ""})),
                         doctest::Contains("option --paths needs a value"), std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli({"--map", "", "--scen", "s.scen", "--agents", "1",
                                    "--solver", "cbs", "--output", "o.json"}),
                         doctest::Contains("option --map needs a value"), std::invalid_argument);
}

TEST_CASE("an option given twice is rejected") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--seed", "1", "--seed", "2"})),
                         doctest::Contains("option --seed given more than once"),
                         std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--w", "1", "--w", "1"})),
                         doctest::Contains("option --w given more than once"), std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli({"--map", "a.map", "--map", "b.map", "--scen", "s.scen",
                                    "--agents", "1", "--solver", "cbs", "--output", "o.json"}),
                         doctest::Contains("option --map given more than once"),
                         std::invalid_argument);
}

TEST_CASE("a non-integer agent count is rejected") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(base_required(), {"--agents", "5x"})),
                         doctest::Contains("option --agents needs an integer, got '5x'"),
                         std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli(with_args(base_required(), {"--agents", "abc"})),
                         doctest::Contains("option --agents needs an integer, got 'abc'"),
                         std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli(with_args(base_required(), {"--agents", "1.5"})),
                         doctest::Contains("option --agents needs an integer, got '1.5'"),
                         std::invalid_argument);
}

TEST_CASE("agent counts below one are rejected") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(base_required(), {"--agents", "0"})),
                         doctest::Contains("option --agents must be at least 1"),
                         std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli(with_args(base_required(), {"--agents", "-2"})),
                         doctest::Contains("option --agents must be at least 1"),
                         std::invalid_argument);
}

TEST_CASE("a negative seed is rejected") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--seed", "-1"})),
                         doctest::Contains("option --seed must not be negative"),
                         std::invalid_argument);
    CHECK(parse_cli(with_args(required(), {"--seed", "0"})).seed == 0);
}

TEST_CASE("a malformed bound is rejected") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--w", "1.0.0"})),
                         doctest::Contains("option --w needs a number, got '1.0.0'"),
                         std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--w", "abc"})),
                         doctest::Contains("option --w needs a number, got 'abc'"),
                         std::invalid_argument);
}

TEST_CASE("a bound below one is rejected") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--w", "0.99"})),
                         doctest::Contains("option --w must be at least 1"), std::invalid_argument);
    CHECK(parse_cli(with_args(required(), {"--w", "1"})).w == doctest::Approx(1.0));
}

TEST_CASE("a non-positive timeout is rejected") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--timeout", "0"})),
                         doctest::Contains("option --timeout must be positive"),
                         std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--timeout", "-3"})),
                         doctest::Contains("option --timeout must be positive"),
                         std::invalid_argument);
}

TEST_CASE("an infinite timeout is rejected as malformed") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--timeout", "inf"})),
                         doctest::Contains("option --timeout needs a number, got 'inf'"),
                         std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--timeout", "1s"})),
                         doctest::Contains("option --timeout needs a number, got '1s'"),
                         std::invalid_argument);
}

TEST_CASE("an unsupported solver is rejected") {
    CHECK_THROWS_WITH_AS(parse_cli({"--map", "m.map", "--scen", "s.scen", "--agents", "1",
                                    "--solver", "astar", "--output", "o.json"}),
                         doctest::Contains("option --solver must be cbs or ecbs, got 'astar'"),
                         std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli({"--map", "m.map", "--scen", "s.scen", "--agents", "1",
                                    "--solver", "CBS", "--output", "o.json"}),
                         doctest::Contains("option --solver must be cbs or ecbs, got 'CBS'"),
                         std::invalid_argument);
}

TEST_CASE("an unsupported conflict policy is rejected") {
    CHECK_THROWS_WITH_AS(parse_cli(with_args(required(), {"--conflict", "last"})),
                         doctest::Contains(
                             "option --conflict must be first, earliest or cardinal, got 'last'"),
                         std::invalid_argument);
}

TEST_CASE("every conflict policy spelling is accepted") {
    CHECK(parse_cli(with_args(required(), {"--conflict", "first"})).conflict_policy == "first");
    CHECK(parse_cli(with_args(required(), {"--conflict", "earliest"})).conflict_policy ==
          "earliest");
    CHECK(parse_cli(with_args(required(), {"--conflict", "cardinal"})).conflict_policy ==
          "cardinal");
}

TEST_CASE("a missing required option is reported in the documented order") {
    CHECK_THROWS_WITH_AS(parse_cli({"--scen", "s.scen", "--agents", "1", "--solver", "cbs",
                                    "--output", "o.json"}),
                         doctest::Contains("missing required option --map"), std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli({"--map", "m.map", "--agents", "1", "--solver", "cbs",
                                    "--output", "o.json"}),
                         doctest::Contains("missing required option --scen"), std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli({"--map", "m.map", "--scen", "s.scen", "--solver", "cbs",
                                    "--output", "o.json"}),
                         doctest::Contains("missing required option --agents"),
                         std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli({"--map", "m.map", "--scen", "s.scen", "--agents", "1",
                                    "--output", "o.json"}),
                         doctest::Contains("missing required option --solver"),
                         std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli({"--map", "m.map", "--scen", "s.scen", "--agents", "1",
                                    "--solver", "cbs"}),
                         doctest::Contains("missing required option --output"),
                         std::invalid_argument);
    CHECK_THROWS_WITH_AS(parse_cli({}),
                         doctest::Contains("missing required option --map"), std::invalid_argument);
}

TEST_CASE("--help alone returns immediately with help set") {
    const CliOptions opts = parse_cli({"--help"});

    CHECK(opts.help);
    CHECK(opts.map_path.empty());
    CHECK(opts.scen_path.empty());
    CHECK(opts.agents == 0);
    CHECK(opts.solver.empty());
    CHECK(opts.output_path.empty());
}

TEST_CASE("-h wins over invalid arguments") {
    CHECK(parse_cli({"-h", "--bogus"}).help);
    CHECK(parse_cli({"--bogus", "-h"}).help);
    CHECK(parse_cli({"--agents", "-h"}).help);
    CHECK(parse_cli({"--seed", "1", "--seed", "2", "--help"}).help);
}

TEST_CASE("the usage text mentions every option") {
    const std::string usage = cli_usage();

    CHECK(usage.rfind("Usage: mapf_solver --map", 0) == 0);
    CHECK(usage.find("--map") != std::string::npos);
    CHECK(usage.find("--scen") != std::string::npos);
    CHECK(usage.find("--agents") != std::string::npos);
    CHECK(usage.find("--solver") != std::string::npos);
    CHECK(usage.find("--w") != std::string::npos);
    CHECK(usage.find("--conflict") != std::string::npos);
    CHECK(usage.find("--seed") != std::string::npos);
    CHECK(usage.find("--timeout") != std::string::npos);
    CHECK(usage.find("--output") != std::string::npos);
    CHECK(usage.find("--paths") != std::string::npos);
    CHECK(usage.find("--help") != std::string::npos);
    CHECK(usage.find("-h") != std::string::npos);
}

}  // namespace mapf
