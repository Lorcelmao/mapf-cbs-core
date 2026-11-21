#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef PSAPI_VERSION
#define PSAPI_VERSION 2
#endif
#endif

#include "app/peak_memory.h"

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__APPLE__) || defined(__unix__) || defined(__linux__)
#include <sys/resource.h>
#endif

namespace mapf {

std::uint64_t peak_memory_bytes() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters,
                             static_cast<DWORD>(sizeof(counters))) == 0) {
        return 0;
    }
    return static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
#elif defined(__APPLE__)
    struct rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0;
    }
    // ru_maxrss is already reported in bytes on Apple platforms.
    return static_cast<std::uint64_t>(usage.ru_maxrss);
#elif defined(__unix__) || defined(__linux__)
    struct rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0;
    }
    // ru_maxrss is reported in kilobytes on Linux and other POSIX systems.
    return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024u;
#else
    return 0;
#endif
}

}  // namespace mapf
