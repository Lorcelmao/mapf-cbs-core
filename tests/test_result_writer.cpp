#include <doctest.h>

#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "app/result_writer.h"
#include "core/grid.h"

TEST_CASE("json_escape copies plain text unchanged") {
    CHECK(mapf::json_escape("hello world") == "hello world");
    CHECK(mapf::json_escape("") == "");
}

TEST_CASE("json_escape escapes quotes and backslashes") {
    CHECK(mapf::json_escape("\"") == R"(\")");
    CHECK(mapf::json_escape(R"(C:\maps\a.map)") == R"(C:\\maps\\a.map)");
}

TEST_CASE("json_escape escapes newline tab and carriage return") {
    CHECK(mapf::json_escape("a\nb") == R"(a\nb)");
    CHECK(mapf::json_escape("a\tb") == R"(a\tb)");
    CHECK(mapf::json_escape("a\rb") == R"(a\rb)");
}

TEST_CASE("json_escape writes other control bytes as four digit escapes") {
    CHECK(mapf::json_escape(std::string(1, '\x01')) == R"(\u0001)");
    CHECK(mapf::json_escape(std::string(1, static_cast<char>(0x1f))) == R"(\u001f)");
}

TEST_CASE("json_escape keeps bytes of utf8 text untouched") {
    const std::string utf8 = "\xC3\xA9";
    CHECK(mapf::json_escape(utf8) == utf8);
}

TEST_CASE("status_text names each solver status") {
    mapf::RunRecord record;
    record.validity = "valid";

    record.result.status = mapf::SolveStatus::Solved;
    CHECK(mapf::status_text(record) == "solved");

    record.result.status = mapf::SolveStatus::Timeout;
    CHECK(mapf::status_text(record) == "timeout");

    record.result.status = mapf::SolveStatus::NoSolution;
    CHECK(mapf::status_text(record) == "no_solution");

    record.result.status = mapf::SolveStatus::Error;
    CHECK(mapf::status_text(record) == "error");
}

TEST_CASE("status_text reports a rejected solved run as invalid") {
    mapf::RunRecord record;
    record.result.status = mapf::SolveStatus::Solved;
    record.validity = "invalid";
    CHECK(mapf::status_text(record) == "invalid");

    record.result.status = mapf::SolveStatus::Timeout;
    CHECK(mapf::status_text(record) == "timeout");
}

TEST_CASE("build type and compiler description are always present") {
    const std::string type = mapf::build_type();
    CHECK((type == "Release" || type == "Debug"));
    CHECK_FALSE(mapf::compiler_description().empty());
}

TEST_CASE("write_result_json writes every field of a solved record in order") {
    mapf::RunRecord record;
    record.options.map_path = "maps/a.map";
    record.options.scen_path = "scen/a.scen";
    record.options.agents = 10;
    record.options.solver = "ecbs";
    record.options.w = 1.05;
    record.options.conflict_policy = "earliest";
    record.options.seed = 3;
    record.options.timeout_s = 60.0;
    record.validity = "valid";
    record.peak_memory_bytes = 1048576;

    record.result.status = mapf::SolveStatus::Solved;
    record.result.soc = 123;
    record.result.makespan = 20;
    record.result.lb_independent = 100;
    record.result.lb_frontier = 110;
    record.result.lb_certified = 110;
    record.result.lb_source = "frontier";
    record.result.frontier_consistent = true;
    record.result.hl_expanded = 5;
    record.result.hl_generated = 11;
    record.result.ll_calls = 21;
    record.result.ll_expanded = 1000;
    record.result.ll_generated = 2000;
    record.result.runtime_s = 0.5;
    record.result.runtime_lowlevel_s = 0.25;
    record.result.runtime_detect_s = 0.125;

    std::ostringstream out;
    mapf::write_result_json(out, record);

    std::string expected = "{\n";
    expected += "  \"schema_version\": 1,\n";
    expected += "  \"map\": \"maps/a.map\",\n";
    expected += "  \"scen\": \"scen/a.scen\",\n";
    expected += "  \"agents\": 10,\n";
    expected += "  \"solver\": \"ecbs\",\n";
    expected += "  \"w\": 1.05,\n";
    expected += "  \"conflict_policy\": \"earliest\",\n";
    expected += "  \"seed\": 3,\n";
    expected += "  \"timeout_s\": 60,\n";
    expected += "  \"status\": \"solved\",\n";
    expected += "  \"error\": \"\",\n";
    expected += "  \"validity\": \"valid\",\n";
    expected += "  \"soc\": 123,\n";
    expected += "  \"makespan\": 20,\n";
    expected += "  \"lb_independent\": 100,\n";
    expected += "  \"lb_frontier\": 110,\n";
    expected += "  \"lb_certified\": 110,\n";
    expected += "  \"lb_source\": \"frontier\",\n";
    expected += "  \"frontier_consistent\": true,\n";
    expected += "  \"hl_expanded\": 5,\n";
    expected += "  \"hl_generated\": 11,\n";
    expected += "  \"ll_calls\": 21,\n";
    expected += "  \"ll_expanded\": 1000,\n";
    expected += "  \"ll_generated\": 2000,\n";
    expected += "  \"runtime_s\": 0.5,\n";
    expected += "  \"runtime_lowlevel_s\": 0.25,\n";
    expected += "  \"runtime_detect_s\": 0.125,\n";
    expected += "  \"peak_memory_bytes\": 1048576,\n";
    expected += "  \"compiler\": \"" + mapf::compiler_description() + "\",\n";
    expected += "  \"build_type\": \"" + mapf::build_type() + "\"\n";
    expected += "}\n";

    CHECK(out.str() == expected);
}

TEST_CASE("write_result_json prints empty optionals as null") {
    mapf::RunRecord record;
    record.result.status = mapf::SolveStatus::Timeout;

    std::ostringstream out;
    mapf::write_result_json(out, record);
    const std::string text = out.str();

    CHECK(text.find("  \"status\": \"timeout\",\n") != std::string::npos);
    CHECK(text.find("  \"soc\": null,\n") != std::string::npos);
    CHECK(text.find("  \"lb_source\": null,\n") != std::string::npos);
    CHECK(text.find("  \"frontier_consistent\": null,\n") != std::string::npos);
    CHECK(text.find("  \"validity\": \"unchecked\",\n") != std::string::npos);
    CHECK(text.compare(0, 2, "{\n") == 0);
    REQUIRE(text.size() >= 2);
    CHECK(text.compare(text.size() - 2, 2, "}\n") == 0);
}

TEST_CASE("write_result_json prints a non finite runtime as null") {
    mapf::RunRecord record;
    record.result.status = mapf::SolveStatus::Solved;
    record.result.runtime_s = std::numeric_limits<double>::infinity();

    std::ostringstream out;
    mapf::write_result_json(out, record);

    CHECK(out.str().find("  \"runtime_s\": null,\n") != std::string::npos);
}

TEST_CASE("write_result_json escapes the error message") {
    mapf::RunRecord record;
    record.result.status = mapf::SolveStatus::Error;
    record.result.error = "bad \"thing\"\n";

    std::ostringstream out;
    mapf::write_result_json(out, record);

    CHECK(out.str().find("  \"error\": \"bad \\\"thing\\\"\\n\",\n") != std::string::npos);
}

TEST_CASE("write_paths prints agent index followed by x,y pairs") {
    const mapf::Grid grid = mapf::grid_from_rows({"....", "....", "...."});
    const std::vector<mapf::Path> paths = {{0, 1, 5}, {11, 11}};

    std::ostringstream out;
    mapf::write_paths(out, grid, paths);

    CHECK(out.str() == "agents 2\n0 0,0 1,0 1,1\n1 3,2 3,2\n");
}

TEST_CASE("write_paths prints a bare index for an empty path") {
    const mapf::Grid grid = mapf::grid_from_rows({"....", "....", "...."});
    const std::vector<mapf::Path> paths = {{}, {5}};

    std::ostringstream out;
    mapf::write_paths(out, grid, paths);

    CHECK(out.str() == "agents 2\n0\n1 1,1\n");
}
