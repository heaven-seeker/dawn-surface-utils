#pragma once

#include <webgpu/webgpu_cpp.h>
#include <memory>
#include <string>
#include <utility>

namespace dawn_utils {

enum class SurfaceError {
    none,
    invalid_argument,
    unsupported_platform,
    native_window_unavailable,
    native_setup_failed,
    dawn_creation_failed
};

// Owns the Dawn surface and any platform attachment it needs. The application
// must keep the window alive and destroy this owner on the window's main thread.
class WindowSurface {
public:
    WindowSurface() = default;
    explicit WindowSurface(wgpu::Surface handle, std::shared_ptr<void> attachment = {})
        : attachment_(std::move(attachment)), handle_(std::move(handle)) {}

    WindowSurface(const WindowSurface&) = delete;
    WindowSurface& operator=(const WindowSurface&) = delete;
    WindowSurface(WindowSurface&&) noexcept = default;
    WindowSurface& operator=(WindowSurface&& other) noexcept {
        if (this != &other) {
            reset();
            attachment_ = std::move(other.attachment_);
            handle_ = std::move(other.handle_);
        }
        return *this;
    }

    [[nodiscard]] const wgpu::Surface& handle() const noexcept { return handle_; }
    [[nodiscard]] explicit operator bool() const noexcept { return handle_.Get() != nullptr; }
    void reset() noexcept {
        handle_ = nullptr;
        attachment_.reset();
    }

private:
    // Reverse member destruction releases Dawn before the native attachment.
    std::shared_ptr<void> attachment_;
    wgpu::Surface handle_;
};

struct SurfaceResult {
    WindowSurface surface;
    SurfaceError error = SurfaceError::none;
    std::string message;

    [[nodiscard]] explicit operator bool() const noexcept {
        return error == SurfaceError::none && static_cast<bool>(surface);
    }
};

} // namespace dawn_utils
