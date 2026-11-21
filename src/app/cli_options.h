#pragma once

#include <string>
#include <vector>

namespace mapf {

// Command-line options of the mapf_solver executable.
struct CliOptions {
    std::string map_path;                   // --map <file.map>      required
    std::string scen_path;                  // --scen <file.scen>    required
    int agents = 0;                         // --agents <k>          required, k >= 1
    std::string solver;                     // --solver cbs|ecbs     required
    double w = 1.0;                         // --w <bound>           >= 1
    std::string conflict_policy = "first";  // --conflict first|earliest|cardinal
    int seed = 0;                           // --seed <n>            >= 0
    double timeout_s = 60.0;                // --timeout <seconds>   > 0
    std::string output_path;                // --output <file.json>  required
    std::string paths_path;                 // --paths <file>        optional
    bool help = false;                      // --help or -h
};

// Parses the arguments that follow the program name. Options have the form
// `--name value`. Throws std::invalid_argument, with a message that names the
// option, for an unknown option, a missing value, a repeated option, a value
// that is malformed or out of range, or a missing required option. When
// --help or -h is present it returns immediately with help == true and checks
// nothing else.
CliOptions parse_cli(const std::vector<std::string>& args);

// Multi-line usage text listing every option.
std::string cli_usage();

}  // namespace mapf
