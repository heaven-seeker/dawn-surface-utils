#include <dawn_utils/surface.hpp>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <type_traits>

#if defined(TEST_GLFW)
#include <dawn_utils/glfw_surface.hpp>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#endif
#if defined(TEST_SDL)
#include <dawn_utils/sdl_surface.hpp>
#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#endif

template<class Value>
void expect(const Value& value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

void check_capabilities(const wgpu::Instance& instance, const wgpu::Surface& surface) {
    struct Pending { bool ready = false; wgpu::Adapter adapter; };
    auto pending = std::make_shared<Pending>();
    wgpu::RequestAdapterOptions options{};
    options.compatibleSurface = surface;
    const auto request = instance.RequestAdapter(&options, wgpu::CallbackMode::WaitAnyOnly,
        [pending](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView) {
            if (status == wgpu::RequestAdapterStatus::Success) pending->adapter = std::move(adapter);
            pending->ready = true;
        });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (!pending->ready && std::chrono::steady_clock::now() < deadline) {
        instance.WaitAny(request, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    expect(pending->ready, "Adapter request timed out");
    expect(pending->adapter, "No compatible adapter");
    wgpu::SurfaceCapabilities capabilities;
    expect(surface.GetCapabilities(pending->adapter, &capabilities) == wgpu::Status::Success,
           "Could not query surface capabilities");
    expect(capabilities.formatCount != 0 && capabilities.presentModeCount != 0,
           "Surface has no presentation formats or modes");
}

void check_owner() {
    static_assert(!std::is_copy_constructible_v<dawn_utils::WindowSurface>);
    static_assert(std::is_nothrow_move_constructible_v<dawn_utils::WindowSurface>);
    int destroyed = 0;
    auto attachment = [&destroyed] {
        return std::shared_ptr<void>(new int, [&destroyed](void* value) {
            delete static_cast<int*>(value);
            ++destroyed;
        });
    };
    dawn_utils::WindowSurface first({}, attachment());
    dawn_utils::WindowSurface second({}, attachment());
    second = std::move(first);
    expect(destroyed == 1, "Move assignment must free the previous attachment");
    first.reset();
    expect(destroyed == 1, "A moved-from owner must not free transferred resources");
    second.reset();
    second.reset();
    expect(destroyed == 2, "Each attachment must be freed exactly once");
}

#if defined(TEST_GLFW)
void check_glfw(const wgpu::Instance& instance, bool null_platform) {
    struct Session {
        ~Session() { glfwTerminate(); }
    } session;
    if (null_platform) glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_NULL);
    expect(glfwInit(), "GLFW initialization failed");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window(
        glfwCreateWindow(80, 80, "Dawn test", nullptr, nullptr), glfwDestroyWindow);
    expect(window, "GLFW window creation failed");
    expect(dawn_utils::create_glfw_surface(instance, nullptr).error ==
           dawn_utils::SurfaceError::invalid_argument, "Null window must be rejected");
    expect(!dawn_utils::create_glfw_surface({}, window.get()), "Null instance must be rejected");
    for (int iteration = 0; iteration != 2; ++iteration) {
        auto result = dawn_utils::create_glfw_surface(instance, window.get(), "GLFW test");
        if (null_platform) {
            expect(result.error == dawn_utils::SurfaceError::unsupported_platform,
                   "GLFW null platform must be rejected");
        } else {
            expect(result, result.message.c_str());
            auto owner = std::move(result.surface);
            expect(!result.surface, "Moved-from surface must be empty");
            check_capabilities(instance, owner.handle());
        }
    }
}
#endif

#if defined(TEST_SDL)
void check_sdl(const wgpu::Instance& instance, bool dummy) {
    struct Session {
        ~Session() { SDL_Quit(); }
    } session;
    SDL_SetMainReady();
    if (dummy) expect(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"), "Could not select dummy driver");
    expect(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
        SDL_CreateWindow("Dawn test", 80, 80, SDL_WINDOW_HIGH_PIXEL_DENSITY), SDL_DestroyWindow);
    expect(window, SDL_GetError());
    expect(!dawn_utils::create_sdl_surface({}, window.get()), "Null instance must be rejected");
    expect(dawn_utils::create_sdl_surface(instance, nullptr).error ==
           dawn_utils::SurfaceError::invalid_argument, "Null window must be rejected");
    for (int iteration = 0; iteration != 2; ++iteration) {
        auto result = dawn_utils::create_sdl_surface(instance, window.get(), "SDL test");
        if (dummy) {
            expect(result.error == dawn_utils::SurfaceError::unsupported_platform,
                   "SDL dummy driver must be rejected");
        } else {
            expect(result, result.message.c_str());
            dawn_utils::WindowSurface owner;
            owner = std::move(result.surface);
            expect(!result.surface, "Moved-from surface must be empty");
            check_capabilities(instance, owner.handle());
            owner.reset();
        }
    }
}
#endif

int main(int argc, char** argv) {
    try {
        const std::string_view mode = argc > 1 ? argv[1] : "owner";
        if (mode == "owner") {
            check_owner();
        } else {
            auto instance = wgpu::CreateInstance();
            expect(instance, "Dawn instance creation failed");
#if defined(TEST_GLFW)
            if (mode == "glfw" || mode == "glfw-null") check_glfw(instance, mode == "glfw-null");
            else
#endif
#if defined(TEST_SDL)
            if (mode == "sdl" || mode == "sdl-dummy") check_sdl(instance, mode == "sdl-dummy");
            else
#endif
            throw std::runtime_error("Unknown test mode");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
