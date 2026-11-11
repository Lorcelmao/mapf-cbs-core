#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mapf {

// Passable 4-neighbours of one cell, always in the order up, down, left, right.
// The fixed order keeps every search that iterates over it deterministic.
struct Neighbors {
    std::array<int, 4> cells{};
    int count = 0;

    const int* begin() const { return cells.data(); }
    const int* end() const { return cells.data() + count; }
};

// A 4-connected grid map. A cell is the integer row * width + col, with row 0
// at the top and col 0 on the left.
class Grid {
public:
    // `passable` has one entry per cell in row-major order; non-zero means the
    // cell can be entered. Throws std::invalid_argument on inconsistent sizes.
    Grid(int height, int width, std::vector<std::uint8_t> passable);

    int height() const { return height_; }
    int width() const { return width_; }
    int size() const { return height_ * width_; }
    int passable_count() const { return passable_count_; }

    int cell(int row, int col) const { return row * width_ + col; }
    int row(int cell) const { return cell / width_; }
    int col(int cell) const { return cell % width_; }

    bool in_bounds(int row, int col) const {
        return row >= 0 && row < height_ && col >= 0 && col < width_;
    }
    bool passable(int cell) const { return passable_[cell] != 0; }

    Neighbors neighbors(int cell) const;

private:
    int height_;
    int width_;
    int passable_count_;
    std::vector<std::uint8_t> passable_;
};

// MovingAI terrain: '.', 'G' and 'S' are passable; '@', 'O', 'T' and 'W' are
// obstacles. Throws std::invalid_argument for any other character.
bool is_passable_terrain(char terrain);

// Builds a grid from equally long rows of MovingAI terrain characters.
// Throws std::invalid_argument when rows are empty, ragged, or hold an unknown
// character.
Grid grid_from_rows(const std::vector<std::string>& rows);

}  // namespace mapf
