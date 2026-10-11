#include "renderer/Renderer.h"
#include "Scene.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

// NOTE: this uses the old fixed-function OpenGL (glBegin / glEnd) so it works
// with no extra libraries. Later we can swap it for shaders and Vulkan.

static const float kOrange[3] = {1.0f, 0.60f, 0.10f};

static void faceColor(const SceneObject& o, int fi, const Mat4& model, RenderMode mode,
                      bool editing, float out[3]) {
    const Mesh& m = o.mesh;
    Vec3 n = normalize(model.dir(m.faceNormal(fi)));   // direction the face points, in the world

    if (mode == RenderMode::Normals) {
        out[0] = n.x * 0.5f + 0.5f; out[1] = n.y * 0.5f + 0.5f; out[2] = n.z * 0.5f + 0.5f;
    } else if (mode == RenderMode::Unlit) {
        for (int k = 0; k < 3; k++) out[k] = o.color[k];
    } else {
        static const Vec3 light = normalize(Vec3{0.4f, 0.8f, 0.5f});
        float shade = 0.30f + 0.70f * std::max(0.0f, dot(n, light));
        for (int k = 0; k < 3; k++) out[k] = o.color[k] * shade;
    }
    if (editing && fi < (int)m.fsel.size() && m.fsel[fi])
        for (int k = 0; k < 3; k++) out[k] = out[k] * 0.4f + kOrange[k] * 0.6f;
}

static void drawFaces(const SceneObject& o, const Mat4& model, RenderMode mode, bool editing) {
    const Mesh& m = o.mesh;
    glEnable(GL_POLYGON_OFFSET_FILL);   // push faces back a little so lines on top stay visible
    glPolygonOffset(1.0f, 1.0f);
    glBegin(GL_TRIANGLES);
    for (size_t fi = 0; fi < m.f.size(); fi++) {
        float c[3];
        faceColor(o, (int)fi, model, mode, editing, c);
        glColor3fv(c);
        const Face& face = m.f[fi];
        for (size_t k = 1; k + 1 < face.size(); k++) {
            glVertex3fv(&m.v[face[0]].x);
            glVertex3fv(&m.v[face[k]].x);
            glVertex3fv(&m.v[face[k + 1]].x);
        }
    }
    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL);
}

static void drawOutlines(const Mesh& m, float r, float g, float b, float width) {
    glLineWidth(width);
    glColor3f(r, g, b);
    for (const Face& face : m.f) {
        glBegin(GL_LINE_LOOP);
        for (int id : face) glVertex3fv(&m.v[id].x);
        glEnd();
    }
}

static void drawPoints(const Mesh& m, float r, float g, float b, float size) {
    glPointSize(size);
    glColor3f(r, g, b);
    glBegin(GL_POINTS);
    for (const Vec3& p : m.v) glVertex3fv(&p.x);
    glEnd();
}

// Edit mode: dark edges, orange selected edges, and the points
static void drawEditOverlay(const Mesh& m) {
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    for (const Face& face : m.f)
        for (size_t k = 0; k < face.size(); k++) {
            int a = face[k], b = face[(k + 1) % face.size()];
            bool sel = a < (int)m.vsel.size() && b < (int)m.vsel.size() && m.vsel[a] && m.vsel[b];
            if (sel) glColor3fv(kOrange); else glColor3f(0.05f, 0.05f, 0.05f);
            glVertex3fv(&m.v[a].x);
            glVertex3fv(&m.v[b].x);
        }
    glEnd();

    glDisable(GL_DEPTH_TEST);   // points show through the surface
    glPointSize(7.0f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < m.v.size(); i++) {
        bool sel = i < m.vsel.size() && m.vsel[i];
        if (sel) glColor3fv(kOrange); else glColor3f(0.9f, 0.9f, 0.9f);
        glVertex3fv(&m.v[i].x);
    }
    glEnd();
    glEnable(GL_DEPTH_TEST);
}

static void drawFloorGrid() {
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = -10; i <= 10; i++) {
        if (i == 0) continue;
        glColor3f(0.28f, 0.33f, 0.42f);
        glVertex3f((float)i, 0, -10); glVertex3f((float)i, 0, 10);
        glVertex3f(-10, 0, (float)i); glVertex3f(10, 0, (float)i);
    }
    glColor3f(0.85f, 0.25f, 0.25f);   // X axis = red
    glVertex3f(-10, 0, 0); glVertex3f(10, 0, 0);
    glColor3f(0.25f, 0.45f, 0.90f);   // Z axis = blue
    glVertex3f(0, 0, -10); glVertex3f(0, 0, 10);
    glEnd();
}

bool Renderer::init() {
    glClearColor(0.10f, 0.15f, 0.25f, 1.0f);
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
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(cam.proj((float)m_width / (float)m_height).m);
    glMatrixMode(GL_MODELVIEW);

    glLoadMatrixf(view.m);
    drawFloorGrid();

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
                drawOutlines(o.mesh, o.color[0], o.color[1], o.color[2], 1.0f);
                break;
            case RenderMode::Points:
                drawPoints(o.mesh, o.color[0], o.color[1], o.color[2], 4.0f);
                break;
            case RenderMode::SolidWire:
                drawFaces(o, model, RenderMode::Solid, editing);
                drawOutlines(o.mesh, 0.05f, 0.05f, 0.05f, 1.0f);
                break;
            default:   // Solid, Normals, Unlit
                drawFaces(o, model, mode, editing);
                break;
        }

        if (editing) drawEditOverlay(o.mesh);
        else if (isSelected) drawOutlines(o.mesh, kOrange[0], kOrange[1], kOrange[2], 2.0f);   // orange outline
    }

    glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);
    glLineWidth(1.0f);
    glPointSize(1.0f);
}

void Renderer::shutdown() {}