#include <dawn_utils/sdl_surface.hpp>
#include "native_surface.hpp"
#include <SDL3/SDL.h>
#include <string_view>

#if !SDL_VERSION_ATLEAST(3, 2, 0)
#error "dawn_utils requires SDL 3.2+"
#endif

#if defined(SDL_PLATFORM_MACOS) || defined(SDL_PLATFORM_IOS)
#include <SDL3/SDL_metal.h>
#endif

namespace dawn_utils {

SurfaceResult create_sdl_surface(const wgpu::Instance& instance, SDL_Window* window,
                                 wgpu::StringView label) {
    if (!instance || !window) {
        return detail::failure(SurfaceError::invalid_argument, "An instance and an SDL window are required.");
    }
    const auto properties = SDL_GetWindowProperties(window);
    const char* current = SDL_GetCurrentVideoDriver();
    if (!properties || !current) {
        return detail::failure(SurfaceError::native_window_unavailable, SDL_GetError());
    }
    const std::string_view driver(current);
    const auto pointer = [properties](const char* key) {
        return SDL_GetPointerProperty(properties, key, nullptr);
    };

#if defined(SDL_PLATFORM_MACOS) || defined(SDL_PLATFORM_IOS)
    if (driver == "cocoa" || driver == "uikit") {
        std::shared_ptr<void> view(SDL_Metal_CreateView(window), [](void* value) {
            if (value) SDL_Metal_DestroyView(value);
        });
        if (!view) return detail::failure(SurfaceError::native_setup_failed, SDL_GetError());
        wgpu::SurfaceSourceMetalLayer metal;
        metal.layer = SDL_Metal_GetLayer(view.get());
        if (!metal.layer) return detail::failure(SurfaceError::native_setup_failed, SDL_GetError());
        return detail::connect(instance, metal, label, std::move(view));
    }
#endif
    if (driver == "windows") {
        wgpu::SurfaceSourceWindowsHWND win32;
        win32.hwnd = pointer(SDL_PROP_WINDOW_WIN32_HWND_POINTER);
        win32.hinstance = pointer(SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER);
        if (win32.hwnd && win32.hinstance) return detail::connect(instance, win32, label);
    } else if (driver == "x11") {
        wgpu::SurfaceSourceXlibWindow xlib;
        xlib.display = pointer(SDL_PROP_WINDOW_X11_DISPLAY_POINTER);
        xlib.window = static_cast<uint64_t>(SDL_GetNumberProperty(
            properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
        if (xlib.display && xlib.window) return detail::connect(instance, xlib, label);
    } else if (driver == "wayland") {
        wgpu::SurfaceSourceWaylandSurface wayland;
        wayland.display = pointer(SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER);
        wayland.surface = pointer(SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER);
        if (wayland.display && wayland.surface) return detail::connect(instance, wayland, label);
    } else if (driver == "android") {
        wgpu::SurfaceSourceAndroidNativeWindow android;
        android.window = pointer(SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER);
        if (android.window) return detail::connect(instance, android, label);
    } else {
        return detail::failure(SurfaceError::unsupported_platform,
                               "Unsupported SDL video driver: " + std::string(driver));
    }
    return detail::failure(SurfaceError::native_window_unavailable,
                           "SDL native window handles are unavailable; show the window first.");
}

} // namespace dawn_utils
