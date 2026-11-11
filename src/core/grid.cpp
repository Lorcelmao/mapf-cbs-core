#include "core/grid.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace mapf {

Grid::Grid(int height, int width, std::vector<std::uint8_t> passable)
    : height_(height), width_(width), passable_count_(0), passable_(std::move(passable)) {
    if (height <= 0 || width <= 0) {
        throw std::invalid_argument("grid height and width must be positive");
    }
    const long long cells = static_cast<long long>(height) * width;
    if (cells > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("grid has too many cells");
    }
    if (static_cast<long long>(passable_.size()) != cells) {
        throw std::invalid_argument("grid needs exactly height * width cells");
    }
    for (const std::uint8_t value : passable_) {
        if (value != 0) {
            ++passable_count_;
        }
    }
}

Neighbors Grid::neighbors(int cell) const {
    Neighbors result;
    const int r = row(cell);
    const int c = col(cell);
    if (r > 0 && passable(cell - width_)) {
        result.cells[result.count++] = cell - width_;
    }
    if (r + 1 < height_ && passable(cell + width_)) {
        result.cells[result.count++] = cell + width_;
    }
    if (c > 0 && passable(cell - 1)) {
        result.cells[result.count++] = cell - 1;
    }
    if (c + 1 < width_ && passable(cell + 1)) {
        result.cells[result.count++] = cell + 1;
    }
    return result;
}

bool is_passable_terrain(char terrain) {
    switch (terrain) {
        case '.':
        case 'G':
        case 'S':
            return true;
        case '@':
        case 'O':
        case 'T':
        case 'W':
            return false;
        default:
            throw std::invalid_argument(std::string("unknown terrain character '") + terrain + "'");
    }
}

Grid grid_from_rows(const std::vector<std::string>& rows) {
    if (rows.empty() || rows.front().empty()) {
        throw std::invalid_argument("grid needs at least one non-empty row");
    }
    const std::size_t width = rows.front().size();
    std::vector<std::uint8_t> passable;
    passable.reserve(rows.size() * width);
    for (std::size_t r = 0; r < rows.size(); ++r) {
        if (rows[r].size() != width) {
            throw std::invalid_argument("row " + std::to_string(r) + " has " +
                                        std::to_string(rows[r].size()) + " cells, expected " +
                                        std::to_string(width));
        }
        for (std::size_t c = 0; c < width; ++c) {
            try {
                passable.push_back(is_passable_terrain(rows[r][c]) ? 1 : 0);
            } catch (const std::invalid_argument& error) {
                throw std::invalid_argument("row " + std::to_string(r) + ", col " +
                                            std::to_string(c) + ": " + error.what());
            }
        }
    }
    if (rows.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        width > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("grid has too many cells");
    }
    return Grid(static_cast<int>(rows.size()), static_cast<int>(width), std::move(passable));
}

}  // namespace mapf
