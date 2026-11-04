#pragma once

#include "surface.hpp"

struct GLFWwindow;

namespace dawn_utils {

// GLFW 3.4+, initialized on the main thread. Create windows with GLFW_NO_API.
[[nodiscard]] SurfaceResult create_glfw_surface(
    const wgpu::Instance& instance, GLFWwindow* window, wgpu::StringView label = {});

} // namespace dawn_utils
