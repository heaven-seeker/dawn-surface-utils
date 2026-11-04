# Dawn Utils

C++20 utilities for creating native WebGPU surfaces with Dawn.
This folder is a standalone project: copy it into your repository or use it
with `add_subdirectory()`. There are two independent libraries:

| CMake target | Function | Window systems |
| --- | --- | --- |
| `dawn_utils::glfw` | `create_glfw_surface()` | Windows, macOS, X11, Wayland |
| `dawn_utils::sdl` | `create_sdl_surface()` | Windows, macOS, iOS, Android, X11, Wayland |

Requires CMake 3.21+, a C++20 compiler, modern Dawn with
`<webgpu/webgpu_cpp.h>`, and GLFW 3.4+ and/or SDL 3.2+.

## Add to an application

```cmake
find_package(Threads REQUIRED)
find_package(Dawn CONFIG REQUIRED)

# Choose the window libraries you need. Both default to ON.
set(DAWN_UTILS_WITH_GLFW ON CACHE BOOL "")
set(DAWN_UTILS_WITH_SDL OFF CACHE BOOL "")
add_subdirectory(dawn_utils)
target_link_libraries(my_app PRIVATE dawn_utils::glfw)
```

Dependencies may already exist as CMake targets, or be installed packages found
through `CMAKE_PREFIX_PATH`. Dawn targets `dawn::webgpu_dawn`, `webgpu_dawn`, and
`webgpu` are detected automatically. Set `DAWN_UTILS_DAWN_TARGET` to use another
existing target providing Dawn's C API and C++ headers. When linking `dawn_proc`
instead of monolithic Dawn, initialize its native procedure table yourself.
No dependencies are downloaded or included in this folder.

To use SDL instead, enable `DAWN_UTILS_WITH_SDL`, disable
`DAWN_UTILS_WITH_GLFW`, and link `dawn_utils::sdl`. Link both targets when needed.

## Create a surface

```cpp
#include <dawn_utils/glfw_surface.hpp>
#include <iostream>

auto result = dawn_utils::create_glfw_surface(instance, window, "Main window");
if (!result) {
    std::cerr << result.message << '\n';
    return;
}
const wgpu::Surface& surface = result.surface.handle();
// Request an adapter with compatibleSurface, then configure and render.
```

For SDL:

```cpp
#include <dawn_utils/sdl_surface.hpp>

auto result = dawn_utils::create_sdl_surface(instance, window, "Main window");
```

`instance` is a `wgpu::Instance`; `window` is a `GLFWwindow*` or `SDL_Window*`.
The label is optional. Errors are returned in the result as an enum and a
message; the utilities do not use a global error string. Dawn's own validation
messages are still reported by Dawn.

Create GLFW windows with `GLFW_CLIENT_API = GLFW_NO_API`. For SDL, initialize
`SDL_INIT_VIDEO` and use a window without an SDL_Renderer or OpenGL/Vulkan
context. Call surface creation and destruction on the main thread. Show
Wayland windows before querying their native handles.

## Ownership

`WindowSurface` is movable and cannot be copied. It releases the Dawn surface
automatically. On Apple, the SDL implementation also owns the Metal view and
destroys it after releasing the Dawn surface. No private window property or
separate cleanup call is required.

Keep the owner alive while rendering. The reference from `handle()` is borrowed;
do not retain a copy beyond its owner's lifetime. Keep the window alive until
the owner has been destroyed or reset:

```cpp
result.surface.reset();
glfwDestroyWindow(window); // Or SDL_DestroyWindow(window).
```

Release acquired frame textures and unconfigure a configured surface before
resetting it. Do not use another rendering API on that window or attach multiple
SDL Metal views at the same time. Reset the previous owner before recreating a
surface for the same SDL window on Apple.

The utilities do not create a GPU device or render loop. Use pixel dimensions
from `glfwGetFramebufferSize()` / `SDL_GetWindowSizeInPixels()` for surface
configuration, reconfigure on resize, and skip zero-sized frames. On Android,
recreate the surface when the OS replaces the native window.

## Build, examples, and tests

```sh
cmake -S dawn_utils -B build-dawn-utils \
    -DCMAKE_PREFIX_PATH="/path/to/dawn;/path/to/window/library" \
    -DDAWN_UTILS_BUILD_EXAMPLES=ON -DDAWN_UTILS_BUILD_TESTS=ON
cmake --build build-dawn-utils --config Release
ctest --test-dir build-dawn-utils -C Release --output-on-failure
```

Examples `dawn_glfw_example` and `dawn_sdl_example` create a window and a surface,
then wait for close; they do not render. On Windows, known runtime DLL
dependencies are copied beside the executables. Match your compiler, runtime,
and build configuration to the Dawn SDK you use.

Tests check actual surface capabilities with a compatible GPU, recreation,
invalid arguments, unsupported backends, and ownership moves/reset. Native
tests require a display and a Dawn-supported GPU.

For source-built GLFW, Linux backend defaults follow `GLFW_BUILD_X11` and
`GLFW_BUILD_WAYLAND`. For installed GLFW, both default to ON. Disable absent
native APIs using `DAWN_UTILS_GLFW_X11` / `DAWN_UTILS_GLFW_WAYLAND`. The GLFW
implementation requires Xlib/Xrandr headers and/or wayland-client with
pkg-config. SDL needs no X11/Wayland headers or private backend definitions.

Windows validation uses Dawn `55c03af2b97acd886d6d65bd345c214c4408ed40`, GLFW
3.5.1, and SDL 3.4.14. Linux, macOS, iOS, and Android need native validation on
their respective systems. Emscripten is outside this project's scope.
