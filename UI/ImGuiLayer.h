#pragma once
struct GLFWwindow;

namespace ImGuiLayer {
    void init(GLFWwindow* window);
    void beginFrame();
    void endFrame();
    void shutdown();
}