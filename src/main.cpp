#include <GLFW/glfw3.h>
#include <cstdio>
#include "renderer/Renderer.h"

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
    glfwSwapInterval(1);  // vsync

    Renderer renderer;
    renderer.init();

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    renderer.resize(w, h);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glfwGetFramebufferSize(window, &w, &h);
        renderer.resize(w, h);

        renderer.renderFrame();
        glfwSwapBuffers(window);
    }

    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}