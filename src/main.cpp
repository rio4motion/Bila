#include <GLFW/glfw3.h>
#include <cstdio>
#include "renderer/Renderer.h"
#include "ImGuiLayer.h"                // NEW

int main() {
    if (!glfwInit()) {
        std::printf("Failed to start GLFW\n");
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Bila", nullptr, nullptr);
    if (!window) {
        std::printf("Failed to create window\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    Renderer renderer;
    renderer.init();
    ImGuiLayer::init(window);          // NEW

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    renderer.resize(w, h);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glfwGetFramebufferSize(window, &w, &h);
        renderer.resize(w, h);

        renderer.renderFrame();
        ImGuiLayer::beginFrame();      // NEW
        ImGuiLayer::endFrame();        // NEW
        glfwSwapBuffers(window);
    }

    ImGuiLayer::shutdown();            // NEW (counted as the 4th line above)
    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}