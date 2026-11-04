#include <dawn_utils/glfw_surface.hpp>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* window = glfwCreateWindow(800, 600, "Dawn utilities: GLFW", nullptr, nullptr);
    auto instance = wgpu::CreateInstance();
    auto result = dawn_utils::create_glfw_surface(instance, window, "GLFW example");
    const bool success = static_cast<bool>(result);
    if (success) {
        std::cout << "Surface ready. This example does not render.\n";
        while (!glfwWindowShouldClose(window)) glfwWaitEvents();
    } else {
        std::cerr << result.message << '\n';
    }
    result.surface.reset();
    if (window) glfwDestroyWindow(window);
    glfwTerminate();
    return success ? 0 : 1;
}
