#ifndef HAYWIRE_GUI_HPP
#define HAYWIRE_GUI_HPP
#include "world.hpp"
#include <SDL2/SDL.h>
#include <memory>
#include <chrono>

namespace haywire
{

struct window
{
    using window_resource_type   =
        std::unique_ptr<SDL_Window,   decltype(&SDL_DestroyWindow)>;
    using renderer_resource_type =
        std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;

    window()
    : origin_x_(0), origin_y_(0), cell_size_(4), world_(),
      window_(SDL_CreateWindow("haywire", 0, 0, 640, 480, SDL_WINDOW_RESIZABLE),
              &SDL_DestroyWindow),
      renderer_(SDL_CreateRenderer(window_.get(), -1, 0), &SDL_DestroyRenderer)
    {}
    ~window() = default;
    window(const window&) = delete;
    window(window&&)      = default;
    window& operator=(const window&) = delete;
    window& operator=(window&&)      = default;

    bool update()
    {
        const auto fps60 = std::chrono::system_clock::now() +
                           std::chrono::milliseconds(17);

        this->world_.update();
        this->draw();

        const bool continues = this->handle_event();

        while(std::chrono::system_clock::now() < fps60) {}

        return continues;
    }

    void draw()
    {
        SDL_SetRenderDrawColor(renderer_.get(), 0,0,0,0xFF);
        SDL_RenderClear(renderer_.get());

        int w, h;
        SDL_GetWindowSize(window_.get(), &w, &h);
        const int window_width  = w;
        const int window_height = h;

        const std::size_t cell_begin_x = origin_x_ / cell_size_;
        const std::size_t cell_begin_y = origin_y_ / cell_size_;
        const std::size_t cell_end_x   = (origin_x_ + window_width)  / cell_size_;
        const std::size_t cell_end_y   = (origin_y_ + window_height) / cell_size_;

        for(std::size_t y = cell_begin_y; y < cell_end_y; ++y)
        {
            for(std::size_t x = cell_begin_x; x < cell_end_x; ++x)
            {
                switch(std::as_const(world_)(x, y))
                {
                    case state::vacuum:
                    {
                        continue;
                    }
                    case state::wire:
                    {
                        SDL_SetRenderDrawColor(renderer_.get(), 0xFF, 0xFF, 0, 0xFF);
                        break;
                    }
                    case state::head:
                    {
                        SDL_SetRenderDrawColor(renderer_.get(), 0xFF, 0x00, 0, 0xFF);
                    }
                    case state::tail:
                    {
                        SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0xFF, 0xFF);
                        break;
                    }
                }
                SDL_Rect rect;
                rect.x = x * cell_size_ - origin_x_;
                rect.y = y * cell_size_ - origin_y_;
                rect.w = cell_size_;
                rect.h = cell_size_;
                SDL_RenderDrawRect(renderer_.get(), &rect);
            }
        }
        SDL_RenderPresent(renderer_.get());
        return ;
    }

    bool handle_event()
    {
        SDL_Event event;
        SDL_PollEvent(&event);

        switch(event.type)
        {
            case SDL_QUIT: {return false;}
        }
        return true;
    }

  private:

    std::size_t            origin_x_, origin_y_;
    std::size_t            cell_size_;
    world                  world_;
    window_resource_type   window_;
    renderer_resource_type renderer_;
};


} // haywire
#endif// HAYWIRE_WORLD_HPP
