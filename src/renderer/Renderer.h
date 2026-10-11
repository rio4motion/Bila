#pragma once

class Renderer {
public:
    bool init();                 // call once, after the OpenGL context exists
    void resize(int w, int h);   // call when the size changes
    void renderFrame();          // draws one frame
    void shutdown();             // cleanup

private:
    int m_width = 0;
    int m_height = 0;
};