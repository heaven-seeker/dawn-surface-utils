#pragma once

#include "surface.hpp"

struct SDL_Window;

namespace dawn_utils {

// SDL 3.2+, initialized on the main thread. Do not attach another renderer.
// On Apple, the returned owner also owns the SDL Metal view.
[[nodiscard]] SurfaceResult create_sdl_surface(
    const wgpu::Instance& instance, SDL_Window* window, wgpu::StringView label = {});

} // namespace dawn_utils
