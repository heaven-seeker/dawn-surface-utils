#pragma once

#include <dawn_utils/surface.hpp>

namespace dawn_utils::detail {

inline SurfaceResult failure(SurfaceError error, std::string message) {
    return {{}, error, std::move(message)};
}

template<class NativeSource>
SurfaceResult connect(const wgpu::Instance& instance, const NativeSource& source,
                      wgpu::StringView label, std::shared_ptr<void> attachment = {}) {
    wgpu::SurfaceDescriptor descriptor{};
    descriptor.label = label;
    descriptor.nextInChain = &source;
    auto handle = instance.CreateSurface(&descriptor);
    if (!handle) {
        return failure(SurfaceError::dawn_creation_failed,
                       "Dawn could not create a surface; inspect its validation diagnostics.");
    }
    return {WindowSurface(std::move(handle), std::move(attachment)), SurfaceError::none, {}};
}

} // namespace dawn_utils::detail
