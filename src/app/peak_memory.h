#pragma once

#include <cstdint>

namespace mapf {

// Largest amount of physical memory this process has used so far, in bytes
// (peak working set on Windows, peak resident set size on Linux and macOS).
// Returns 0 when the platform cannot report it.
std::uint64_t peak_memory_bytes();

}  // namespace mapf
