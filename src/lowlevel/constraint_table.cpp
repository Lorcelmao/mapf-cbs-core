#include "lowlevel/constraint_table.h"

#include <algorithm>
#include <stdexcept>

namespace mapf {

namespace {

std::uint64_t vertex_key(int cell, int timestep) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cell)) << 32) |
           static_cast<std::uint32_t>(timestep);
}

}  // namespace

std::size_t ConstraintTable::EdgeKeyHash::operator()(const EdgeKey& key) const {
    // Multiplicative mixing so that the three small integers spread over all bits.
    constexpr std::uint64_t kMultiplier = 0x9E3779B97F4A7C15ULL;
    std::uint64_t hash = static_cast<std::uint32_t>(key.from);
    hash = hash * kMultiplier + static_cast<std::uint32_t>(key.to);
    hash = hash * kMultiplier + static_cast<std::uint32_t>(key.timestep);
    hash ^= hash >> 32;
    return static_cast<std::size_t>(hash);
}

ConstraintTable::ConstraintTable(int agent, int goal) : agent_(agent), goal_(goal) {}

void ConstraintTable::add(const Constraint& constraint) {
    if (constraint.agent != agent_) {
        return;
    }
    if (constraint.timestep < 0) {
        throw std::invalid_argument("constraint timestep must not be negative");
    }
    if (constraint.type == ConstraintType::Vertex) {
        vertex_blocks_.insert(vertex_key(constraint.to, constraint.timestep));
        if (constraint.to == goal_) {
            goal_hold_time_ = std::max(goal_hold_time_, constraint.timestep + 1);
        }
    } else {
        if (constraint.timestep == 0) {
            throw std::invalid_argument("an edge constraint needs an arrival time of at least 1");
        }
        edge_blocks_.insert(EdgeKey{constraint.from, constraint.to, constraint.timestep});
    }
    latest_time_ = std::max(latest_time_, constraint.timestep);
}

bool ConstraintTable::is_vertex_blocked(int cell, int timestep) const {
    return timestep <= latest_time_ && vertex_blocks_.count(vertex_key(cell, timestep)) != 0;
}

bool ConstraintTable::is_edge_blocked(int from, int to, int timestep) const {
    return timestep <= latest_time_ && !edge_blocks_.empty() &&
           edge_blocks_.count(EdgeKey{from, to, timestep}) != 0;
}

}  // namespace mapf
