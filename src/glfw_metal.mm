#include <dawn_utils/glfw_surface.hpp>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#import <QuartzCore/CAMetalLayer.h>

namespace dawn_utils::detail {

void* cocoa_layer(GLFWwindow* window) {
    @autoreleasepool {
        NSWindow* cocoa = glfwGetCocoaWindow(window);
        NSView* content = cocoa.contentView;
        if (!content) return nullptr;
        content.wantsLayer = YES;
        if (![content.layer isKindOfClass:CAMetalLayer.class]) {
            content.layer = [CAMetalLayer layer];
        }
        content.layer.contentsScale = cocoa.backingScaleFactor;
        return (__bridge void*)content.layer;
    }
}

} // namespace dawn_utils::detail
