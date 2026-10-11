#include <GLFW/glfw3.h>
#include <cstdio>
#include "renderer/Renderer.h"
#include "ImGuiLayer.h"
#include "Viewport.h"                  // NEW

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
    ImGuiLayer::init(window);

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    renderer.resize(w, h);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glfwGetFramebufferSize(window, &w, &h);
        renderer.resize(w, h);

        ImGuiLayer::beginFrame();      // builds the panels and reads the mouse / keyboard
        Viewport::update();            // NEW: camera, selecting, extrude... (changed order!)
        renderer.renderFrame();        // draws the 3D scene
        ImGuiLayer::endFrame();        // draws the panels on top
        glfwSwapBuffers(window);
    }

    ImGuiLayer::shutdown();
    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}