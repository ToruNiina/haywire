#ifndef HAYWIRE_GUI_HPP
#define HAYWIRE_GUI_HPP
#include "world.hpp"
#include <SDL.h>
#include <stdexcept>
#include <string>
#include <memory>
#include <chrono>
#include <iostream>

namespace haywire
{

struct sdl_resource
{
    sdl_resource(): should_quit_(true)
    {
        if(SDL_Init(SDL_INIT_EVERYTHING) != 0)
        {
            using std::literals::string_literals::operator""s;
            throw std::runtime_error("Error while initializing SDL"s +
                                     SDL_GetError());
        }
    }
    ~sdl_resource()
    {
        if(should_quit_)
        {
            SDL_Quit();
        }
    }

    sdl_resource(const sdl_resource&) = delete;
    sdl_resource& operator=(const sdl_resource&) = delete;

    sdl_resource(sdl_resource&& other): should_quit_(true)
    {
        other.should_quit_ = false;
    }
    sdl_resource& operator=(sdl_resource&& other)
    {
        this->should_quit_ = true;
        other.should_quit_ = false;
        return *this;
    }

  private:
    bool should_quit_;
};

struct window
{
    using window_resource_type   =
        std::unique_ptr<SDL_Window,   decltype(&SDL_DestroyWindow)>;
    using renderer_resource_type =
        std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
    using sdl_resource_type = sdl_resource;

    window(): window(640, 480, 20) {}

    window(std::size_t w, std::size_t h, std::size_t c)
    : origin_x_(0), origin_y_(0), window_width_(w), window_height_(h),
      cell_size_(c), world_(w/c+1, h/c+1), resource_(),
      window_(SDL_CreateWindow("haywire", 0, 0, w, h, SDL_WINDOW_RESIZABLE),
              &SDL_DestroyWindow),
      renderer_(SDL_CreateRenderer(window_.get(), -1, 0), &SDL_DestroyRenderer)
    {
        SDL_GetMouseState(&mouse_prev_x_, &mouse_prev_y_);

        assert(origin_x_ + window_width_  < world_.width()  * cell_size_);
        assert(origin_y_ + window_height_ < world_.height() * cell_size_);
    }
    ~window() = default;
    window(const window&) = delete;
    window(window&&)      = default;
    window& operator=(const window&) = delete;
    window& operator=(window&&)      = default;

    bool update()
    {
        const auto fps60 = std::chrono::system_clock::now() +
                           std::chrono::milliseconds(16);

        this->world_.update();
        this->draw();

        if(not this->handle_event()){return false;}

        while(std::chrono::system_clock::now() < fps60)
        {
            this->draw();
            if(not this->handle_event()){return false;}
        }
        return true;
    }

    void draw()
    {
        SDL_SetRenderDrawColor(renderer_.get(), 0,0,0,0xFF);
        SDL_RenderClear(renderer_.get());

        int w, h;
        SDL_GetWindowSize(window_.get(), &w, &h);
        window_width_  = w;
        window_height_ = h;

        const std::size_t cell_begin_x = origin_x_ / cell_size_;
        const std::size_t cell_begin_y = origin_y_ / cell_size_;
        const std::size_t cell_end_x   = (origin_x_ + window_width_)  / cell_size_;
        const std::size_t cell_end_y   = (origin_y_ + window_height_) / cell_size_;

        const int border = (5 <= cell_size_) ? 1 : 0;

        SDL_Rect rect;
        rect.w = cell_size_- 2 * border;
        rect.h = cell_size_- 2 * border;
        for(std::size_t y = cell_begin_y; y < cell_end_y; ++y)
        {
            for(std::size_t x = cell_begin_x; x < cell_end_x; ++x)
            {
                const auto cell_x = x * cell_size_ - origin_x_;
                const auto cell_y = y * cell_size_ - origin_y_;

                switch(std::as_const(world_)(x, y))
                {
                    case state::vacuum:
                    {
                        continue;
                    }
                    case state::wire:
                    {
                        SDL_SetRenderDrawColor(renderer_.get(), 0xFF, 0xFF, 0x00, 0xFF);
                        break;
                    }
                    case state::head:
                    {
                        SDL_SetRenderDrawColor(renderer_.get(), 0xFF, 0x00, 0x00, 0xFF);
                        break;
                    }
                    case state::tail:
                    {
                        SDL_SetRenderDrawColor(renderer_.get(), 0x00, 0x00, 0xFF, 0xFF);
                        break;
                    }
                }
                rect.x = cell_x + border;
                rect.y = cell_y + border;
                SDL_RenderFillRect(renderer_.get(), &rect);
            }
        }
        SDL_RenderPresent(renderer_.get());
        return ;
    }

    bool handle_event()
    {
        int w, h;
        SDL_GetWindowSize(window_.get(), &w, &h);
        window_width_  = w;
        window_height_ = h;

        SDL_Event event;
        SDL_PollEvent(&event);

        switch(event.type)
        {
            case SDL_QUIT: {return false;}
            case SDL_MOUSEWHEEL:
            {
                std::int32_t cell_size = this->cell_size_;
                cell_size += event.wheel.y;
                this->cell_size_ = std::max(1, cell_size);

                while(world_.width() * cell_size_ < origin_x_ + window_width_)
                {
                    world_.expand_width(world::direction::plus);
                }
                while(world_.height() * cell_size_ < origin_y_ + window_height_)
                {
                    world_.expand_height(world::direction::plus);
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                this->is_mouse_button_down_ = true;
                break;
            }
            case SDL_MOUSEBUTTONUP:
            {
                if(not is_mouse_dragging_)
                {
                    const std::int32_t x = (event.button.x + origin_x_) / cell_size_;
                    const std::int32_t y = (event.button.y + origin_y_) / cell_size_;

                    switch(std::as_const(world_)(x, y))
                    {
                        case state::vacuum: {world_(x, y) = state::wire;   break;}
                        case state::wire:   {world_(x, y) = state::head;   break;}
                        case state::head:   {world_(x, y) = state::tail;   break;}
                        case state::tail:   {world_(x, y) = state::vacuum; break;}
                    }
                }
                this->drag_x_ = 0;
                this->drag_y_ = 0;
                this->mouse_prev_x_ = event.button.x;
                this->mouse_prev_y_ = event.button.y;
                this->is_mouse_button_down_ = false;
                this->is_mouse_dragging_    = false;
                break;
            }
            case SDL_MOUSEMOTION:
            {
                if(is_mouse_button_down_)
                {
                    this->drag_x_ -= event.motion.x - mouse_prev_x_;
                    this->drag_y_ -= event.motion.y - mouse_prev_y_;

                    if(5 <= std::abs(drag_x_) || 5 <= std::abs(drag_y_))
                    {
                        this->is_mouse_dragging_ = true;

                        this->origin_x_ += drag_x_;
                        this->origin_y_ += drag_y_;
                        this->drag_x_ = 0;
                        this->drag_y_ = 0;

                        while(origin_x_ < 0)
                        {
                            world_.expand_width(world::direction::minus);
                            this->origin_x_ += chunk::width * cell_size_;
                        }
                        while(world_.width() * cell_size_ <= origin_x_ + window_width_)
                        {
                            world_.expand_width(world::direction::plus);
                        }

                        while(origin_y_ < 0)
                        {
                            world_.expand_height(world::direction::minus);
                            this->origin_y_ += chunk::height * cell_size_;
                        }
                        while(world_.height() * cell_size_ <= origin_y_ + window_height_)
                        {
                            world_.expand_height(world::direction::plus);
                        }
                    }
                }
                mouse_prev_x_ = event.motion.x;
                mouse_prev_y_ = event.motion.y;
                break;
            }
        }
        return true;
    }

  private:

    bool is_mouse_button_down_ = false;
    bool is_mouse_dragging_    = false;
    std::int32_t drag_x_ = 0,   drag_y_ = 0;
    std::int32_t mouse_prev_x_, mouse_prev_y_;
    std::int32_t origin_x_, origin_y_;
    std::size_t  window_width_, window_height_;
    std::size_t            cell_size_;
    world                  world_;
    sdl_resource_type      resource_;
    window_resource_type   window_;
    renderer_resource_type renderer_;
};


} // haywire
#endif// HAYWIRE_WORLD_HPP
