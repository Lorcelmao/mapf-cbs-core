#include <doctest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "app/peak_memory.h"

TEST_CASE("peak_memory_bytes reports a positive value") {
    CHECK(mapf::peak_memory_bytes() > 0);
}

TEST_CASE("peak memory never shrinks between two consecutive readings") {
    const std::uint64_t first = mapf::peak_memory_bytes();
    const std::uint64_t second = mapf::peak_memory_bytes();
    CHECK(second >= first);
}

TEST_CASE("touching every page of a 64 MiB buffer raises the peak to 60 MiB") {
    const std::size_t kPageBytes = 4096;
    const std::size_t kBytes = 64u * 1024u * 1024u;

    std::vector<unsigned char> buffer(kBytes, 0);
    std::uint64_t sum = 0;
    for (std::size_t offset = 0; offset < kBytes; offset += kPageBytes) {
        buffer[offset] = 1;
        sum += buffer[offset];
    }
    // 64 MiB / 4096 bytes per page = 16384 written bytes of value 1.
    CHECK(sum == 16384ull);

    const std::uint64_t after = mapf::peak_memory_bytes();
    CHECK(after >= 60ull * 1024ull * 1024ull);
}
