#include <dawn_utils/sdl_surface.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    auto* window = SDL_CreateWindow("Dawn utilities: SDL3", 800, 600, SDL_WINDOW_HIGH_PIXEL_DENSITY);
    auto instance = wgpu::CreateInstance();
    auto result = dawn_utils::create_sdl_surface(instance, window, "SDL example");
    const bool success = static_cast<bool>(result);
    if (success) {
        std::cout << "Surface ready. This example does not render.\n";
        SDL_Event event;
        while (SDL_WaitEvent(&event) && event.type != SDL_EVENT_QUIT) {}
    } else {
        std::cerr << result.message << '\n';
    }
    result.surface.reset();
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    return success ? 0 : 1;
}
