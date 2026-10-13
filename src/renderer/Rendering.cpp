#include "renderer/Renderer.h"
#include "Scene.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

// Uses fixed-function OpenGL with vertex arrays (glDrawArrays). It needs no
// extra libraries. Later this can be swapped for shaders / Vulkan.

namespace {

const float kOrange[3] = {1.0f, 0.60f, 0.10f};

// ---------- small helpers ----------
inline void pushPos(std::vector<float>& out, const Vec3& p) {
    out.push_back(p.x); out.push_back(p.y); out.push_back(p.z);
}
inline void pushCol(std::vector<float>& out, const float c[3]) {
    out.push_back(c[0]); out.push_back(c[1]); out.push_back(c[2]);
}

// Sends the buffers to OpenGL in one call. "col" can be null (then the current glColor is used).
void drawArrays(GLenum primitive, const std::vector<float>& pos, const std::vector<float>* col) {
    if (pos.empty()) return;
    glVertexPointer(3, GL_FLOAT, 0, pos.data());
    if (col) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3, GL_FLOAT, 0, col->data());
    }
    glDrawArrays(primitive, 0, (GLsizei)(pos.size() / 3));
    if (col) glDisableClientState(GL_COLOR_ARRAY);
}

// ---------- colors ----------
void faceColor(const SceneObject& o, int fi, const Mat4& model, RenderMode mode,
               bool editing, float out[3]) {
    const Mesh& m = o.mesh;

    if (mode == RenderMode::Unlit) {
        for (int k = 0; k < 3; k++) out[k] = o.color[k];
    } else {
        // only compute the normal when the mode really needs it
        Vec3 n = normalize(model.dir(m.faceNormal(fi)));   // direction the face points, in the world
        if (mode == RenderMode::Normals) {
            out[0] = n.x * 0.5f + 0.5f; out[1] = n.y * 0.5f + 0.5f; out[2] = n.z * 0.5f + 0.5f;
        } else {
            static const Vec3 light = normalize(Vec3{0.4f, 0.8f, 0.5f});
            float shade = 0.30f + 0.70f * std::max(0.0f, dot(n, light));
            for (int k = 0; k < 3; k++) out[k] = o.color[k] * shade;
        }
    }
    if (editing && fi < (int)m.fsel.size() && m.fsel[fi])
        for (int k = 0; k < 3; k++) out[k] = out[k] * 0.4f + kOrange[k] * 0.6f;
}

// ---------- drawing pieces ----------
void drawFaces(const SceneObject& o, const Mat4& model, RenderMode mode, bool editing,
               std::vector<float>& pos, std::vector<float>& col) {
    const Mesh& m = o.mesh;
    pos.clear(); col.clear();

    for (size_t fi = 0; fi < m.f.size(); fi++) {
        float c[3];
        faceColor(o, (int)fi, model, mode, editing, c);
        const Face& face = m.f[fi];
        // a face with n points is cut into n-2 triangles (a fan)
        for (size_t k = 1; k + 1 < face.size(); k++) {
            pushPos(pos, m.v[face[0]]);     pushCol(col, c);
            pushPos(pos, m.v[face[k]]);     pushCol(col, c);
            pushPos(pos, m.v[face[k + 1]]); pushCol(col, c);
        }
    }

    glEnable(GL_POLYGON_OFFSET_FILL);   // push faces back a little so lines on top stay visible
    glPolygonOffset(1.0f, 1.0f);
    drawArrays(GL_TRIANGLES, pos, &col);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void drawOutlines(const Mesh& m, float r, float g, float b, float width, std::vector<float>& pos) {
    pos.clear();
    for (const Face& face : m.f)
        for (size_t k = 0; k < face.size(); k++) {
            pushPos(pos, m.v[face[k]]);
            pushPos(pos, m.v[face[(k + 1) % face.size()]]);
        }
    glLineWidth(width);
    glColor3f(r, g, b);
    drawArrays(GL_LINES, pos, nullptr);
}

void drawPoints(const Mesh& m, float r, float g, float b, float size, std::vector<float>& pos) {
    pos.clear();
    for (const Vec3& p : m.v) pushPos(pos, p);
    glPointSize(size);
    glColor3f(r, g, b);
    drawArrays(GL_POINTS, pos, nullptr);
}

// Edit mode: dark edges, orange selected edges, and the points
void drawEditOverlay(const Mesh& m, std::vector<float>& pos, std::vector<float>& col) {
    const float dark[3] = {0.05f, 0.05f, 0.05f};
    const float light[3] = {0.9f, 0.9f, 0.9f};

    // edges
    pos.clear(); col.clear();
    for (const Face& face : m.f)
        for (size_t k = 0; k < face.size(); k++) {
            int a = face[k], b = face[(k + 1) % face.size()];
            bool sel = a < (int)m.vsel.size() && b < (int)m.vsel.size() && m.vsel[a] && m.vsel[b];
            const float* c = sel ? kOrange : dark;
            pushPos(pos, m.v[a]); pushCol(col, c);
            pushPos(pos, m.v[b]); pushCol(col, c);
        }
    glLineWidth(1.5f);
    drawArrays(GL_LINES, pos, &col);

    // points (drawn through the surface)
    pos.clear(); col.clear();
    for (size_t i = 0; i < m.v.size(); i++) {
        bool sel = i < m.vsel.size() && m.vsel[i];
        pushPos(pos, m.v[i]);
        pushCol(col, sel ? kOrange : light);
    }
    glDisable(GL_DEPTH_TEST);
    glPointSize(7.0f);
    drawArrays(GL_POINTS, pos, &col);
    glEnable(GL_DEPTH_TEST);
}

// Builds the floor grid once.
void buildFloorGrid(std::vector<float>& pos, std::vector<float>& col) {
    pos.clear(); col.clear();
    auto line = [&](Vec3 a, Vec3 b, const float c[3]) {
        pushPos(pos, a); pushCol(col, c);
        pushPos(pos, b); pushCol(col, c);
    };
    const float grid[3] = {0.28f, 0.33f, 0.42f};
    const float xAxis[3] = {0.85f, 0.25f, 0.25f};   // X axis = red
    const float zAxis[3] = {0.25f, 0.45f, 0.90f};   // Z axis = blue
    for (int i = -10; i <= 10; i++) {
        if (i == 0) continue;
        line(Vec3{(float)i, 0, -10}, Vec3{(float)i, 0, 10}, grid);
        line(Vec3{-10, 0, (float)i}, Vec3{10, 0, (float)i}, grid);
    }
    line(Vec3{-10, 0, 0}, Vec3{10, 0, 0}, xAxis);
    line(Vec3{0, 0, -10}, Vec3{0, 0, 10}, zAxis);
}

} // namespace

// ---------- Renderer ----------
bool Renderer::init() {
    glClearColor(0.10f, 0.15f, 0.25f, 1.0f);

    // States that never change are set once here, not every frame.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);
    glEnableClientState(GL_VERTEX_ARRAY);

    buildFloorGrid(m_gridPos, m_gridCol);
    m_pos.reserve(1 << 16);
    m_col.reserve(1 << 16);
    return true;
}

void Renderer::resize(int w, int h) {
    m_width = w;
    m_height = h;
    glViewport(0, 0, w, h);
}

void Renderer::renderFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (m_width == 0 || m_height == 0) return;

    Scene& scene = getScene();
    Camera& cam = getCamera();
    Mat4 view = cam.view();

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(cam.proj((float)m_width / (float)m_height).m);
    glMatrixMode(GL_MODELVIEW);

    glLoadMatrixf(view.m);
    glLineWidth(1.0f);
    drawArrays(GL_LINES, m_gridPos, &m_gridCol);

    for (int i = 0; i < (int)scene.objects.size(); i++) {
        const SceneObject& o = scene.objects[i];
        if (!isMeshType(o.type)) continue;

        Mat4 model = o.model();
        glLoadMatrixf((view * model).m);

        bool isSelected = (i == scene.selected);
        bool editing = isSelected && scene.editMode;
        RenderMode mode = scene.renderMode;

        switch (mode) {
            case RenderMode::Wireframe:
                drawOutlines(o.mesh, o.color[0], o.color[1], o.color[2], 1.0f, m_pos);
                break;
            case RenderMode::Points:
                drawPoints(o.mesh, o.color[0], o.color[1], o.color[2], 4.0f, m_pos);
                break;
            case RenderMode::SolidWire:
                drawFaces(o, model, RenderMode::Solid, editing, m_pos, m_col);
                drawOutlines(o.mesh, 0.05f, 0.05f, 0.05f, 1.0f, m_pos);
                break;
            default:   // Solid, Normals, Unlit
                drawFaces(o, model, mode, editing, m_pos, m_col);
                break;
        }

        if (editing) drawEditOverlay(o.mesh, m_pos, m_col);
        else if (isSelected) drawOutlines(o.mesh, kOrange[0], kOrange[1], kOrange[2], 2.0f, m_pos);   // orange outline
    }

    glDisable(GL_DEPTH_TEST);
    glLineWidth(1.0f);
    glPointSize(1.0f);
}

void Renderer::shutdown() {
    glDisableClientState(GL_VERTEX_ARRAY);
    m_pos.clear();  m_pos.shrink_to_fit();
    m_col.clear();  m_col.shrink_to_fit();
    m_gridPos.clear(); m_gridCol.clear();
}