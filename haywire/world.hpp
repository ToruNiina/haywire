#ifndef HAYWIRE_WORLD_HPP
#define HAYWIRE_WORLD_HPP
#include <extlib/toml11/toml.hpp>
#include <extlib/wad/wad/archive.hpp>
#include <extlib/wad/wad/interface.hpp>
#include <extlib/wad/wad/vector.hpp>
#include <extlib/wad/wad/array.hpp>
#include <extlib/wad/wad/enum.hpp>
#include <algorithm>
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

    template<typename Archiver>
    bool save(Archiver& arc) const
    {
        return wad::save<wad::type::map>(arc, "cells", cells);
    }
    template<typename Archiver>
    bool load(Archiver& arc)
    {
        return wad::load<wad::type::map>(arc, "cells", cells);
    }

    toml::array into_toml() const
    {
        toml::array retval(width * height);
        std::transform(cells.begin(), cells.end(), retval.begin(),
            [](const state& s) noexcept -> toml::value {
                return toml::integer(static_cast<std::uint8_t>(s));
            });
        return retval;
    }
    void from_toml(const toml::value& v)
    {
        const auto tmp = toml::get<std::array<std::uint8_t, width * height>>(v);
        std::transform(tmp.begin(), tmp.end(), cells.begin(),
                [](const std::uint8_t x) noexcept -> state {
                    return static_cast<state>(x);
                });
        return;
    }

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

    world(std::size_t w, std::size_t h)
        : width_ ((w / chunk::width  + (w % chunk::width  != 0)) * chunk::width),
          height_((h / chunk::height + (h % chunk::height != 0)) * chunk::height),
          width_chunk_ (w / chunk::width  + (w % chunk::width  != 0)),
          height_chunk_(h / chunk::height + (h % chunk::height != 0)),
          chunks_    (width_chunk_ * height_chunk_),
          chunks_buf_(width_chunk_ * height_chunk_)
    {
        assert(width_chunk_  * chunk::width  == width_);
        assert(height_chunk_ * chunk::height == height_);
    }

    world(const toml::value& v)
        : width_ (toml::find<std::size_t>(v, "width")),
          height_(toml::find<std::size_t>(v, "height")),
          width_chunk_ (width_  / chunk::width  + (width_  % chunk::width  != 0)),
          height_chunk_(height_ / chunk::height + (height_ % chunk::height != 0)),
          chunks_    (toml::find<std::vector<chunk>>(v, "chunks")),
          chunks_buf_(chunks_)
    {
        assert(chunks_.size() == width_chunk_ * height_chunk_);
        assert(width_chunk_  * chunk::width  == width_);
        assert(height_chunk_ * chunk::height == height_);
    }

    toml::value into_toml() const
    {
        toml::array tmp(chunks_.size());
        std::transform(chunks_.begin(), chunks_.end(), tmp.begin(),
            [](const auto& ch) -> toml::value {return ch.into_toml();});
        return toml::value{
            {"width", width_}, {"height", height_}, {"chunks", std::move(tmp)}
        };
    }

    template<typename Archiver>
    bool save(Archiver& arc) const
    {
         return wad::save<wad::type::map>(arc,
                "width", width_, "height", height_, "chunks", chunks_);
    }
    template<typename Archiver>
    bool load(Archiver& arc)
    {
        const auto result = wad::load<wad::type::map>(arc,
                "width", width_, "height", height_, "chunks", chunks_);

        width_chunk_  = width_  / chunk::width  + (width_  % chunk::width  != 0),
        height_chunk_ = height_ / chunk::height + (height_ % chunk::height != 0),
        chunks_buf_   = chunks_;
        return result;
    }

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
        chunks_buf_.resize(chunks_.size() + this->height_chunk_);
        std::fill(chunks_buf_.begin(), chunks_buf_.end(), chunk{});

        if(dir == direction::plus)
        {
            for(std::uint32_t y = 0; y < this->height_chunk_; ++y)
            {
                for(std::uint32_t x = 0; x < this->width_chunk_; ++x)
                {
                    chunks_buf_.at((this->width_chunk_ + 1) * y + x) =
                        chunks_.at( this->width_chunk_      * y + x);
                }
            }
        }
        else
        {
            for(std::uint32_t y = 0; y < this->height_chunk_; ++y)
            {
                for(std::uint32_t x = 0; x < this->width_chunk_; ++x)
                {
                    // chunks are expanded to the minus direction
                    chunks_buf_.at((this->width_chunk_ + 1) * y + x + 1) =
                        chunks_.at( this->width_chunk_      * y + x);
                }
            }
        }
        chunks_ = chunks_buf_;
        this->width_chunk_ += 1;
        this->width_        = chunk::width * width_chunk_;
        assert(chunks_.size() == this->width_chunk_ * this->height_chunk_);
        return;
    }
    void expand_height(direction dir)
    {
        chunks_buf_.resize(chunks_.size() + this->width_chunk_);
        std::fill(chunks_buf_.begin(), chunks_buf_.end(), chunk{});

        if(dir == direction::plus)
        {
            for(std::uint32_t y = 0; y < this->height_chunk_; ++y)
            {
                for(std::uint32_t x = 0; x < this->width_chunk_; ++x)
                {
                    chunks_buf_.at(this->width_chunk_ * y + x) =
                        chunks_.at(this->width_chunk_ * y + x);
                }
            }
        }
        else
        {
            for(std::uint32_t y = 0; y < this->height_chunk_; ++y)
            {
                for(std::uint32_t x = 0; x < this->width_chunk_; ++x)
                {
                    // chunks are expanded to the minus direction
                    chunks_buf_.at(this->width_chunk_ * (y+1) + x) =
                        chunks_.at(this->width_chunk_ *  y    + x);
                }
            }
        }
        chunks_ = chunks_buf_;
        this->height_chunk_ += 1;
        this->height_        = chunk::height * height_chunk_;

        assert(chunks_.size() == this->width_chunk_ * this->height_chunk_);
        return;
    }

    std::size_t width()  const noexcept {return width_ ;}
    std::size_t height() const noexcept {return height_;}

  private:
    std::size_t width_, height_, width_chunk_, height_chunk_;
    std::vector<chunk>  chunks_;
    std::vector<chunk>  chunks_buf_;
};

} // haywire
#endif// HAYWIRE_WORLD_HPP
