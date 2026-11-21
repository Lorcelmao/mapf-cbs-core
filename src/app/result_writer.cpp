#include "app/result_writer.h"

#include <cmath>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <string>

namespace mapf {
namespace {

std::string number_text(double value) {
    if (!std::isfinite(value)) {
        return "null";
    }
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(9) << value;
    return stream.str();
}

std::string optional_int_text(const std::optional<int>& value) {
    if (!value.has_value()) {
        return "null";
    }
    return std::to_string(*value);
}

std::string optional_bool_text(const std::optional<bool>& value) {
    if (!value.has_value()) {
        return "null";
    }
    return *value ? "true" : "false";
}

}  // namespace

std::string json_escape(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (char raw : text) {
        const unsigned char c = static_cast<unsigned char>(raw);
        switch (c) {
            case '"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            case '\b':
                escaped += "\\b";
                break;
            case '\f':
                escaped += "\\f";
                break;
            default:
                if (c < 0x20) {
                    static const char kHex[] = "0123456789abcdef";
                    escaped += "\\u00";
                    escaped += kHex[(c >> 4) & 0x0F];
                    escaped += kHex[c & 0x0F];
                } else {
                    escaped += static_cast<char>(c);
                }
                break;
        }
    }
    return escaped;
}

std::string compiler_description() {
#if defined(__clang__)
    return "Clang " + std::to_string(__clang_major__) + "." +
           std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__)
    return "GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." +
           std::to_string(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    return "MSVC " + std::to_string(_MSC_VER);
#else
    return "unknown";
#endif
}

std::string build_type() {
#ifdef NDEBUG
    return "Release";
#else
    return "Debug";
#endif
}

std::string status_text(const RunRecord& record) {
    switch (record.result.status) {
        case SolveStatus::Solved:
            return record.validity == "invalid" ? "invalid" : "solved";
        case SolveStatus::Timeout:
            return "timeout";
        case SolveStatus::NoSolution:
            return "no_solution";
        case SolveStatus::Error:
            return "error";
    }
    return "error";
}

void write_result_json(std::ostream& out, const RunRecord& record) {
    const SolveResult& result = record.result;

    std::ostringstream text;
    text.imbue(std::locale::classic());

    text << "{\n";
    text << "  \"schema_version\": " << kResultSchemaVersion << ",\n";
    text << "  \"map\": \"" << json_escape(record.options.map_path) << "\",\n";
    text << "  \"scen\": \"" << json_escape(record.options.scen_path) << "\",\n";
    text << "  \"agents\": " << record.options.agents << ",\n";
    text << "  \"solver\": \"" << json_escape(record.options.solver) << "\",\n";
    text << "  \"w\": " << number_text(record.options.w) << ",\n";
    text << "  \"conflict_policy\": \"" << json_escape(record.options.conflict_policy) << "\",\n";
    text << "  \"seed\": " << record.options.seed << ",\n";
    text << "  \"timeout_s\": " << number_text(record.options.timeout_s) << ",\n";
    text << "  \"status\": \"" << status_text(record) << "\",\n";
    text << "  \"error\": \"" << json_escape(result.error) << "\",\n";
    text << "  \"validity\": \"" << json_escape(record.validity) << "\",\n";
    text << "  \"soc\": " << optional_int_text(result.soc) << ",\n";
    text << "  \"makespan\": " << optional_int_text(result.makespan) << ",\n";
    text << "  \"lb_independent\": " << optional_int_text(result.lb_independent) << ",\n";
    text << "  \"lb_frontier\": " << optional_int_text(result.lb_frontier) << ",\n";
    text << "  \"lb_certified\": " << optional_int_text(result.lb_certified) << ",\n";
    if (result.lb_source.empty()) {
        text << "  \"lb_source\": null,\n";
    } else {
        text << "  \"lb_source\": \"" << json_escape(result.lb_source) << "\",\n";
    }
    text << "  \"frontier_consistent\": " << optional_bool_text(result.frontier_consistent) << ",\n";
    text << "  \"hl_expanded\": " << result.hl_expanded << ",\n";
    text << "  \"hl_generated\": " << result.hl_generated << ",\n";
    text << "  \"ll_calls\": " << result.ll_calls << ",\n";
    text << "  \"ll_expanded\": " << result.ll_expanded << ",\n";
    text << "  \"ll_generated\": " << result.ll_generated << ",\n";
    text << "  \"runtime_s\": " << number_text(result.runtime_s) << ",\n";
    text << "  \"runtime_lowlevel_s\": " << number_text(result.runtime_lowlevel_s) << ",\n";
    text << "  \"runtime_detect_s\": " << number_text(result.runtime_detect_s) << ",\n";
    text << "  \"peak_memory_bytes\": " << record.peak_memory_bytes << ",\n";
    text << "  \"compiler\": \"" << json_escape(compiler_description()) << "\",\n";
    text << "  \"build_type\": \"" << build_type() << "\"\n";
    text << "}\n";

    out << text.str();
}

void write_paths(std::ostream& out, const Grid& grid, const std::vector<Path>& paths) {
    out << "agents " << paths.size() << "\n";
    for (std::size_t i = 0; i < paths.size(); ++i) {
        out << i;
        for (int cell : paths[i]) {
            out << " " << grid.col(cell) << "," << grid.row(cell);
        }
        out << "\n";
    }
}

}  // namespace mapf
