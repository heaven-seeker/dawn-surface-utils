#include <dawn_utils/glfw_surface.hpp>
#include "native_surface.hpp"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#if GLFW_VERSION_MAJOR < 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR < 4)
#error "dawn_utils requires GLFW 3.4+"
#endif

#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif !defined(__APPLE__)
#if DAWN_UTILS_GLFW_X11
#define GLFW_EXPOSE_NATIVE_X11
#endif
#if DAWN_UTILS_GLFW_WAYLAND
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif
#endif
#if !defined(__APPLE__)
#include <GLFW/glfw3native.h>
#endif

#if defined(__APPLE__)
namespace dawn_utils::detail { void* cocoa_layer(GLFWwindow* window); }
#endif

namespace dawn_utils {

SurfaceResult create_glfw_surface(const wgpu::Instance& instance, GLFWwindow* window,
                                  wgpu::StringView label) {
    if (!instance || !window) {
        return detail::failure(SurfaceError::invalid_argument, "An instance and a GLFW window are required.");
    }
    switch (glfwGetPlatform()) {
#if defined(_WIN32)
    case GLFW_PLATFORM_WIN32: {
        const auto hwnd = glfwGetWin32Window(window);
        if (hwnd) {
            wgpu::SurfaceSourceWindowsHWND win32;
            win32.hwnd = hwnd;
            win32.hinstance = reinterpret_cast<void*>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));
            if (win32.hinstance) return detail::connect(instance, win32, label);
        }
        break;
    }
#elif defined(__APPLE__)
    case GLFW_PLATFORM_COCOA: {
        wgpu::SurfaceSourceMetalLayer metal;
        metal.layer = detail::cocoa_layer(window);
        if (metal.layer) return detail::connect(instance, metal, label);
        break;
    }
#else
#if DAWN_UTILS_GLFW_X11
    case GLFW_PLATFORM_X11: {
        wgpu::SurfaceSourceXlibWindow xlib;
        xlib.display = glfwGetX11Display();
        xlib.window = glfwGetX11Window(window);
        if (xlib.display && xlib.window) return detail::connect(instance, xlib, label);
        break;
    }
#endif
#if DAWN_UTILS_GLFW_WAYLAND
    case GLFW_PLATFORM_WAYLAND: {
        wgpu::SurfaceSourceWaylandSurface wayland;
        wayland.display = glfwGetWaylandDisplay();
        wayland.surface = glfwGetWaylandWindow(window);
        if (wayland.display && wayland.surface) return detail::connect(instance, wayland, label);
        break;
    }
#endif
#endif
    default:
        return detail::failure(SurfaceError::unsupported_platform,
                               "GLFW is uninitialized, or its platform is unsupported/disabled.");
    }
    return detail::failure(SurfaceError::native_window_unavailable,
                           "GLFW could not provide the native window handles.");
}

} // namespace dawn_utils
