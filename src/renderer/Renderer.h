#pragma once
#include <vector>

class Renderer {
public:
    bool init();                  // call once after the OpenGL context exists
    void resize(int w, int h);    // call when the window size changes
    void renderFrame();           // call once per frame
    void shutdown();              // call before closing

private:
    int m_width = 0;
    int m_height = 0;

    // Scratch buffers, reused every frame so we don't allocate memory each time.
    std::vector<float> m_pos;     // x,y,z of every vertex to draw
    std::vector<float> m_col;     // r,g,b of every vertex to draw

    // The floor grid never changes, so it is built once in init().
    std::vector<float> m_gridPos;
    std::vector<float> m_gridCol;
};