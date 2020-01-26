#ifndef HAYWIRE_WORLD_HPP
#define HAYWIRE_WORLD_HPP
#include <array>
#include <vector>
#include <cstdint>
#include <cassert>

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
    static constexpr inline std::size_t width  = 8;
    static constexpr inline std::size_t height = 8;

    std::array<state, width * height> cells;

    chunk() noexcept {cells.fill(state::vacuum);}
    ~chunk() noexcept = default;
    chunk(const chunk&) noexcept = default;
    chunk(chunk&&)      noexcept = default;
    chunk& operator=(const chunk&) noexcept = default;
    chunk& operator=(chunk&&)      noexcept = default;

    state& operator()(const std::size_t x, const std::size_t y)       noexcept
    {
        return cells[width * y + x];
    }
    state  operator()(const std::size_t x, const std::size_t y) const noexcept
    {
        return cells[width * y + x];
    }
};

struct world
{
    enum class direction: std::uint8_t {plus, minus};

    world(std::size_t w, std::size_t h) : width_(w), height_(h),
      width_chunk_(w / chunk::width + 1), height_chunk_(h / chunk::height + 1),
      chunks_    ((w / chunk::width + 1) * (h / chunk::height + 1)),
      chunks_buf_((w / chunk::width + 1) * (h / chunk::height + 1))
    {}

    state& operator()(const std::int32_t x, const std::int32_t y) noexcept
    {
        constexpr std::size_t chunk_width  = chunk::width;
        constexpr std::size_t chunk_height = chunk::height;

        assert(0 <= x && x < width_ && 0 <= y && y < height_);

        const auto x_chk = x / chunk_width;
        const auto x_rem = x % chunk_width;
        const auto y_chk = y / chunk_height;
        const auto y_rem = y % chunk_height;

        return chunks_[width_chunk_ * y_chk + x_chk](x_rem, y_rem);
    }
    state operator()(const std::int32_t x, const std::int32_t y) const noexcept
    {
        constexpr std::size_t chunk_width  = chunk::width;
        constexpr std::size_t chunk_height = chunk::height;

        if(x < 0 || width_ <= x || y < 0 || height_ <= y)
        {
            return state::vacuum;
        }

        const auto x_chk = x / chunk_width;
        const auto x_rem = x % chunk_width;
        const auto y_chk = y / chunk_height;
        const auto y_rem = y % chunk_height;

        return chunks_[width_chunk_ * y_chk + x_chk](x_rem, y_rem);
    }

    chunk& chunk_at(const std::uint32_t x, const std::uint32_t y,
                    const std::nothrow_t&) noexcept
    {
        return chunks_[width_chunk_ * y + x];
    }
    chunk const& chunk_at(const std::uint32_t x, const std::uint32_t y,
                          const std::nothrow_t&) const noexcept
    {
        return chunks_[width_chunk_ * y + x];
    }

    chunk& chunk_at(const std::uint32_t x, const std::uint32_t y)
    {
        return chunks_.at(width_chunk_ * y + x);
    }
    chunk const& chunk_at(const std::uint32_t x, const std::uint32_t y) const
    {
        return chunks_.at(width_chunk_ * y + x);
    }

    void update()
    {
        constexpr std::size_t chunk_width  = chunk::width;
        constexpr std::size_t chunk_height = chunk::height;

        chunks_buf_.resize(chunks_.size());

        for(std::uint32_t y = 0; y < this->height_; ++y)
        {
            const auto y_chk = y / chunk_height;
            const auto y_rem = y % chunk_height;

            for(std::uint32_t x = 0; x < this->width_; ++x)
            {
                const auto x_chk = x / chunk_width;
                const auto x_rem = x % chunk_width;

                const auto& self = *this;// as_const
                switch(self(x, y))
                {
                    case state::vacuum:
                    {
                        chunks_buf_[width_chunk_ * y_chk + x_chk]
                            (x_rem, y_rem) = state::vacuum;
                        break;
                    }
                    case state::wire:
                    {
                        const int count =
                            static_cast<int>(self(x-1, y-1) == state::head) +
                            static_cast<int>(self(x  , y-1) == state::head) +
                            static_cast<int>(self(x+1, y-1) == state::head) +
                            static_cast<int>(self(x-1, y  ) == state::head) +
                            static_cast<int>(self(x  , y  ) == state::head) +
                            static_cast<int>(self(x+1, y  ) == state::head) +
                            static_cast<int>(self(x-1, y+1) == state::head) +
                            static_cast<int>(self(x  , y+1) == state::head) +
                            static_cast<int>(self(x+1, y+1) == state::head);
                        if(count == 1 || count == 2)
                        {
                            chunks_buf_[width_chunk_ * y_chk + x_chk]
                                (x_rem, y_rem) = state::head;
                        }
                        else
                        {
                            chunks_buf_[width_chunk_ * y_chk + x_chk]
                                (x_rem, y_rem) = state::wire;
                        }
                        break;
                    }
                    case state::head:
                    {
                        chunks_buf_[width_chunk_ * y_chk + x_chk]
                            (x_rem, y_rem) = state::tail;
                        break;
                    }
                    case state::tail:
                    {
                        chunks_buf_[width_chunk_ * y_chk + x_chk]
                            (x_rem, y_rem) = state::wire;
                        break;
                    }
                }
            }
        }
        std::swap(chunks_buf_, chunks_);
        return;
    }

    void expand_width(direction dir)
    {
        chunks_buf_.resize(chunks_.size() + this->width_);

        if(dir == direction::plus)
        {
            for(std::uint32_t y = 0; y < this->height_; ++y)
            {
                for(std::uint32_t x = 0; x < this->width_; ++x)
                {
                    chunks_buf_.at(this->width_ * y + x) =
                        chunks_.at(this->width_ * y + x);
                }
            }
        }
        else
        {
            for(std::uint32_t y = 0; y < this->height_; ++y)
            {
                for(std::uint32_t x = 0; x < this->width_; ++x)
                {
                    // chunks are expanded to the minus direction
                    chunks_buf_.at(this->width_ * y + x + 1) =
                        chunks_.at(this->width_ * y + x);
                }
            }
        }
        std::swap(chunks_buf_, chunks_);
        this->width_ += 1;
        chunks_buf_.resize(this->width_ * this->height_);
        return;
    }
    void expand_height(direction dir)
    {
        chunks_buf_.resize(chunks_.size() + this->height_);

        if(dir == direction::plus)
        {
            for(std::uint32_t y = 0; y < this->height_; ++y)
            {
                for(std::uint32_t x = 0; x < this->width_; ++x)
                {
                    chunks_buf_.at(this->width_ * y + x) =
                        chunks_.at(this->width_ * y + x);
                }
            }
        }
        else
        {
            for(std::uint32_t y = 0; y < this->height_; ++y)
            {
                for(std::uint32_t x = 0; x < this->width_; ++x)
                {
                    // chunks are expanded to the minus direction
                    chunks_buf_.at(this->width_ * (y+1) + x) =
                        chunks_.at(this->width_ * y + x);
                }
            }
        }
        std::swap(chunks_buf_, chunks_);
        this->height_ += 1;
        chunks_buf_.resize(this->width_ * this->height_);
        return;
    }

    std::size_t width()  const noexcept {return width_ * chunk::width;}
    std::size_t height() const noexcept {return height_ * chunk::height;}

  private:
    std::size_t width_, height_, width_chunk_, height_chunk_;
    std::vector<chunk>  chunks_;
    std::vector<chunk>  chunks_buf_;
};

} // haywire
#endif// HAYWIRE_WORLD_HPP
