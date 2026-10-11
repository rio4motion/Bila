#include "renderer/Renderer.h"
#include <GLFW/glfw3.h>   // also gives us the basic OpenGL functions

bool Renderer::init() {
    glClearColor(0.10f, 0.15f, 0.25f, 1.0f);  // dark blue (R, G, B, A)
    return true;
}

void Renderer::resize(int w, int h) {
    m_width = w;
    m_height = h;
    glViewport(0, 0, w, h);
}

void Renderer::renderFrame() {
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::shutdown() {}