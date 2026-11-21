#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "app/cli_options.h"
#include "core/grid.h"
#include "core/solve_result.h"
#include "core/types.h"

namespace mapf {

// Everything one run of mapf_solver reports.
struct RunRecord {
    CliOptions options;
    SolveResult result;
    // "unchecked", "valid" or "invalid": verdict of the independent validator.
    std::string validity = "unchecked";
    std::uint64_t peak_memory_bytes = 0;
};

// Version of the JSON layout written by write_result_json.
constexpr int kResultSchemaVersion = 1;

// Escapes `text` for use inside a JSON string literal (without the quotes).
std::string json_escape(const std::string& text);

// Compiler name and version this binary was built with, e.g. "GCC 16.2.0".
std::string compiler_description();

// "Release" when NDEBUG is defined, otherwise "Debug".
std::string build_type();

// The status string of a run: "solved", "timeout", "no_solution" or "error";
// a solved run that the validator rejected reports "invalid" instead.
std::string status_text(const RunRecord& record);

// Writes `record` as one JSON object, one key per line in a fixed order.
void write_result_json(std::ostream& out, const RunRecord& record);

// Writes the paths as text: a line `agents <k>`, then one line per agent with
// its index followed by one `x,y` pair per timestep (x = column, y = row).
void write_paths(std::ostream& out, const Grid& grid, const std::vector<Path>& paths);

}  // namespace mapf
