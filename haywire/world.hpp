#ifndef HAYWIRE_WORLD_HPP
#define HAYWIRE_WORLD_HPP
#include <array>
#include <cstdint>

namespace haywire
{

enum state : std::uint8_t
{
    vacuum = 0u,
    wire   = 1u,
    head   = 2u,
    tail   = 3u,
};

struct chunk
{
    constexpr inline std::size_t width  = 8;
    constexpr inline std::size_t height = 8;

    std::array<state, width * height> cells;

    constexpr state& operator()(const std::size_t x, const std::size_t y)       noexcept
    {
        return cells[width * y + x];
    }
    constexpr state  operator()(const std::size_t x, const std::size_t y) const noexcept
    {
        return cells[width * y + x];
    }
};

struct world
{
    state& operator()(const std::size_t x, const std::size_t y) noexcept
    {
        constexpr std::size_t chunk_width  = chunk::width;
        constexpr std::size_t chunk_height = chunk::height;

        const auto x_chk = x / chunk_width;
        const auto x_rem = x % chunk_width;
        const auto y_chk = y / chunk_height;
        const auto y_rem = y % chunk_height;

        return chunks_[width_ * y_chk + x_chk](x_rem, y_rem);
    }
    state const& operator()(const std::size_t x, const std::size_t y) const noexcept
    {
        constexpr std::size_t chunk_width  = chunk::width;
        constexpr std::size_t chunk_height = chunk::height;

        const auto x_chk = x / chunk_width;
        const auto x_rem = x % chunk_width;
        const auto y_chk = y / chunk_height;
        const auto y_rem = y % chunk_height;

        return chunks_[width_ * y_chk + x_chk](x_rem, y_rem);
    }

    std::size_t width()  const noexcept {return width_;}
    std::size_t height() const noexcept {return height_;}

  private:
    std::size_t width_, height_;
    std::vector<chunk>  chunks_;
};

} // haywire
#endif// HAYWIRE_WORLD_HPP
