#include "Viewport.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace {

enum class Modal { None, Grab, Scale };

Modal g_modal = Modal::None;
std::vector<int> g_ids;       // points being moved
std::vector<Vec3> g_saved;    // where they were before (so Esc can undo)
Vec3 g_center;                // middle of the moved points (used for scaling)
Vec3 g_axis;                  // locked axis in world space
bool g_hasAxis = false;
float g_accX = 0, g_accY = 0; // how far the mouse has moved since the start
bool g_navDrag = false;       // true while orbiting / panning

const float kPi = 3.14159265f;

Mat4 viewProj() {
    ImVec2 d = ImGui::GetIO().DisplaySize;
    float aspect = d.y > 0 ? d.x / d.y : 1.0f;
    return getCamera().proj(aspect) * getCamera().view();
}

// 3D point -> position on screen (in the same units as the mouse)
bool projectPoint(const Mat4& m, Vec3 p, ImVec2 size, float& sx, float& sy, float& depth) {
    float c[4];
    m.clip(p, c);
    if (c[3] <= 1e-4f) return false;   // behind the camera
    sx = (c[0] / c[3] * 0.5f + 0.5f) * size.x;
    sy = (1.0f - (c[1] / c[3] * 0.5f + 0.5f)) * size.y;
    depth = c[2] / c[3];
    return true;
}

bool inTriangle(float px, float py, float ax, float ay, float bx, float by, float cx, float cy,
                float& wa, float& wb, float& wc) {
    float d = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy);
    if (std::fabs(d) < 1e-6f) return false;
    wa = ((by - cy) * (px - cx) + (cx - bx) * (py - cy)) / d;
    wb = ((cy - ay) * (px - cx) + (ax - cx) * (py - cy)) / d;
    wc = 1.0f - wa - wb;
    return wa >= 0 && wb >= 0 && wc >= 0;
}

// Which face is under the mouse? (-1 = none). Picks the one closest to the camera.
int pickFace(const Mesh& m, const Mat4& model, float mx, float my, float& bestDepth) {
    ImVec2 size = ImGui::GetIO().DisplaySize;
    Mat4 mvp = viewProj() * model;
    size_t n = m.v.size();
    std::vector<float> sx(n), sy(n), sd(n);
    std::vector<char> ok(n);
    for (size_t i = 0; i < n; i++) ok[i] = projectPoint(mvp, m.v[i], size, sx[i], sy[i], sd[i]);

    int best = -1;
    bestDepth = 1e9f;
    for (size_t fi = 0; fi < m.f.size(); fi++) {
        const Face& face = m.f[fi];
        for (size_t k = 1; k + 1 < face.size(); k++) {   // faces are cut into triangles
            int a = face[0], b = face[k], c = face[k + 1];
            if (!ok[a] || !ok[b] || !ok[c]) continue;
            float wa, wb, wc;
            if (inTriangle(mx, my, sx[a], sy[a], sx[b], sy[b], sx[c], sy[c], wa, wb, wc)) {
                float d = wa * sd[a] + wb * sd[b] + wc * sd[c];
                if (d < bestDepth) { bestDepth = d; best = (int)fi; }
            }
        }
    }
    return best;
}

// Which point is closest to the mouse (within "radius" pixels)?
int pickVertex(const Mesh& m, const Mat4& model, float mx, float my, float radius) {
    ImVec2 size = ImGui::GetIO().DisplaySize;
    Mat4 mvp = viewProj() * model;
    int best = -1;
    float bestDist = radius * radius;
    for (size_t i = 0; i < m.v.size(); i++) {
        float sx, sy, sd;
        if (!projectPoint(mvp, m.v[i], size, sx, sy, sd)) continue;
        float d = (sx - mx) * (sx - mx) + (sy - my) * (sy - my);
        if (d < bestDist) { bestDist = d; best = (int)i; }
    }
    return best;
}

SceneObject* editObject() {
    Scene& sc = getScene();
    if (!sc.editMode) return nullptr;
    SceneObject* o = sc.selectedObject();
    if (!o || !isMeshType(o->type)) return nullptr;
    o->mesh.fixSelectionSize();
    return o;
}

bool beginModal(Modal kind) {
    Scene& sc = getScene();
    SceneObject* o = editObject();
    if (!o) return false;
    Mesh& m = o->mesh;
    g_ids.clear();
    g_saved.clear();
    g_center = Vec3{};
    for (size_t i = 0; i < m.v.size(); i++)
        if (m.vsel[i]) {
            g_ids.push_back((int)i);
            g_saved.push_back(m.v[i]);
            g_center += m.v[i];
        }
    if (g_ids.empty()) {
        sc.status = "Nothing selected. Click a point or face first.";
        return false;
    }
    g_center = g_center / (float)g_ids.size();
    g_modal = kind;
    g_accX = g_accY = 0;
    g_hasAxis = false;
    return true;
}

void setAxis(Vec3 a, const char* name) {
    g_axis = a;
    g_hasAxis = true;
    getScene().status = std::string("Locked to the ") + name + " axis. Click or Enter = confirm, Esc = cancel";
}

void applyModal(SceneObject& o) {
    ImGuiIO& io = ImGui::GetIO();
    Camera& cam = getCamera();
    Mesh& m = o.mesh;
    g_accX += io.MouseDelta.x;
    g_accY += io.MouseDelta.y;

    if (g_modal == Modal::Grab) {
        // how many world units one pixel is worth, near the target
        float s = 2.0f * cam.distance * std::tan(cam.fovY * 0.5f * kPi / 180.0f) / io.DisplaySize.y;
        Vec3 delta;
        if (g_hasAxis) {
            // follow the mouse along the on-screen line of the locked axis
            float ax = dot(g_axis, cam.right());
            float ay = -dot(g_axis, cam.up());
            float len2 = ax * ax + ay * ay;
            float amount = len2 < 1e-4f ? 0.0f : (g_accX * ax + g_accY * ay) / len2 * s;
            delta = g_axis * amount;
        } else {
            delta = cam.right() * (g_accX * s) - cam.up() * (g_accY * s);
        }
        Vec3 local = o.invModel().dir(delta);   // world movement -> object space
        for (size_t k = 0; k < g_ids.size(); k++) m.v[g_ids[k]] = g_saved[k] + local;
    } else if (g_modal == Modal::Scale) {
        float factor = std::max(0.01f, 1.0f + g_accX / 150.0f);
        for (size_t k = 0; k < g_ids.size(); k++)
            m.v[g_ids[k]] = g_center + (g_saved[k] - g_center) * factor;
    }
}

void cancelModal(SceneObject& o) {
    for (size_t k = 0; k < g_ids.size(); k++) o.mesh.v[g_ids[k]] = g_saved[k];
    g_modal = Modal::None;
    getScene().status = "Cancelled";
}

void pickInEdit(bool shift) {
    Scene& sc = getScene();
    SceneObject* o = editObject();
    if (!o) return;
    Mesh& m = o->mesh;
    ImVec2 mouse = ImGui::GetIO().MousePos;
    Mat4 model = o->model();

    if (sc.selectMode == SelectMode::Vertex) {
        int hit = pickVertex(m, model, mouse.x, mouse.y, 14.0f);
        if (hit >= 0) {
            if (shift) m.vsel[hit] = !m.vsel[hit];
            else { std::fill(m.vsel.begin(), m.vsel.end(), 0); m.vsel[hit] = 1; }
        } else if (!shift) {
            std::fill(m.vsel.begin(), m.vsel.end(), 0);
        }
        m.syncFacesFromVerts();
    } else {
        float depth;
        int hit = pickFace(m, model, mouse.x, mouse.y, depth);
        if (hit >= 0) {
            if (shift) m.fsel[hit] = !m.fsel[hit];
            else { std::fill(m.fsel.begin(), m.fsel.end(), 0); m.fsel[hit] = 1; }
        } else if (!shift) {
            std::fill(m.fsel.begin(), m.fsel.end(), 0);
        }
        m.syncVertsFromFaces();
    }
}

void pickObject() {
    Scene& sc = getScene();
    ImVec2 mouse = ImGui::GetIO().MousePos;
    int best = -1;
    float bestDepth = 1e9f;
    for (size_t i = 0; i < sc.objects.size(); i++) {
        const SceneObject& o = sc.objects[i];
        if (!isMeshType(o.type)) continue;
        float d;
        if (pickFace(o.mesh, o.model(), mouse.x, mouse.y, d) >= 0 && d < bestDepth) {
            bestDepth = d;
            best = (int)i;
        }
    }
    sc.selected = best;
    sc.editMode = false;
    sc.status = best >= 0 ? "Selected " + sc.objects[best].name : "Nothing selected";
}

} // namespace

namespace Viewport {

bool modalActive() { return g_modal != Modal::None; }

void resetCamera() { getCamera() = Camera(); getScene().status = "Camera reset"; }

void focusSelected() {
    Scene& sc = getScene();
    Camera& cam = getCamera();
    SceneObject* o = sc.selectedObject();
    if (!o) return;
    Vec3 p{o->position[0], o->position[1], o->position[2]};
    if (sc.editMode && o->mesh.anyVertSelected()) {
        Vec3 c;
        int n = 0;
        for (size_t i = 0; i < o->mesh.v.size(); i++)
            if (o->mesh.vsel[i]) { c += o->mesh.v[i]; n++; }
        p = o->model().point(c / (float)n);
    }
    cam.target = p;
    float size = std::max(o->scale[0], std::max(o->scale[1], o->scale[2]));
    cam.distance = std::max(2.5f, size * 3.0f);
}

void toggleEditMode() {
    Scene& sc = getScene();
    g_modal = Modal::None;
    if (sc.editMode) {
        sc.editMode = false;
        sc.status = "Object mode";
        return;
    }
    SceneObject* o = sc.selectedObject();
    if (!o || !isMeshType(o->type)) {
        sc.status = "Select a mesh object first (click it)";
        return;
    }
    o->mesh.resetSelection();
    sc.editMode = true;
    sc.status = "Edit mode: click to select, E extrude, G move, S scale";
}

void setSelectMode(SelectMode mode) {
    Scene& sc = getScene();
    sc.selectMode = mode;
    SceneObject* o = editObject();
    if (o && mode == SelectMode::Face) {
        o->mesh.syncFacesFromVerts();
        o->mesh.syncVertsFromFaces();
    }
}

void selectAll() {
    SceneObject* o = editObject();
    if (!o) return;
    std::fill(o->mesh.vsel.begin(), o->mesh.vsel.end(), 1);
    o->mesh.syncFacesFromVerts();
}

void deselectAll() {
    SceneObject* o = editObject();
    if (!o) return;
    o->mesh.resetSelection();
}

void startGrab() {
    if (beginModal(Modal::Grab))
        getScene().status = "Grab: move the mouse. X / Y / Z locks an axis. Click or Enter = confirm, Esc = cancel";
}

void startScale() {
    if (beginModal(Modal::Scale))
        getScene().status = "Scale: move the mouse left or right. Click or Enter = confirm, Esc = cancel";
}

void startExtrude() {
    Scene& sc = getScene();
    SceneObject* o = editObject();
    if (!o) return;
    Mesh& m = o->mesh;
    if (sc.selectMode == SelectMode::Vertex) m.syncFacesFromVerts();
    Vec3 n;
    if (!m.extrudeFaces(n)) {
        sc.status = "Nothing to extrude. Select a face first (press 2, then click a face).";
        return;
    }
    if (beginModal(Modal::Grab)) {
        g_axis = normalize(o->model().dir(n));   // slide along the face direction
        g_hasAxis = true;
        sc.status = "Extrude: move the mouse. Click or Enter = confirm, Esc = cancel";
    }
}

void deleteSelected() {
    Scene& sc = getScene();
    SceneObject* o = editObject();
    if (o) {
        o->mesh.deleteSelected(sc.selectMode == SelectMode::Face);
        sc.status = "Deleted";
    } else {
        sc.removeSelected();
        sc.status = "Object deleted";
    }
}

void update() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x <= 0 || io.DisplaySize.y <= 0) return;
    Scene& sc = getScene();
    Camera& cam = getCamera();

    bool overView = !io.WantCaptureMouse;   // the mouse is over the 3D view, not a panel
    bool rmb = ImGui::IsMouseDown(ImGuiMouseButton_Right);
    bool mmb = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

    // ---- camera: orbit, pan, zoom, fly ----
    if (!g_navDrag && overView &&
        (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)))
        g_navDrag = true;
    if (!rmb && !mmb) g_navDrag = false;

    if (g_navDrag) {
        float s = 2.0f * cam.distance * std::tan(cam.fovY * 0.5f * kPi / 180.0f) / io.DisplaySize.y;
        if (io.KeyShift) cam.pan(io.MouseDelta.x * s, io.MouseDelta.y * s);
        else cam.orbit(io.MouseDelta.x, io.MouseDelta.y);

        if (rmb && !io.WantTextInput) {   // hold right mouse + W A S D Q E to fly around
            float speed = cam.distance * 0.8f * io.DeltaTime * (io.KeyShift ? 3.0f : 1.0f);
            Vec3 move;
            if (ImGui::IsKeyDown(ImGuiKey_W)) move += cam.forward();
            if (ImGui::IsKeyDown(ImGuiKey_S)) move = move - cam.forward();
            if (ImGui::IsKeyDown(ImGuiKey_D)) move += cam.right();
            if (ImGui::IsKeyDown(ImGuiKey_A)) move = move - cam.right();
            if (ImGui::IsKeyDown(ImGuiKey_E)) move += Vec3{0, 1, 0};
            if (ImGui::IsKeyDown(ImGuiKey_Q)) move = move - Vec3{0, 1, 0};
            cam.target += move * speed;
        }
    }
    if (overView && io.MouseWheel != 0.0f) cam.zoom(io.MouseWheel);

    // ---- moving / scaling in progress ----
    if (g_modal != Modal::None) {
        SceneObject* o = editObject();
        if (!o) { g_modal = Modal::None; return; }
        if (ImGui::IsKeyPressed(ImGuiKey_X, false)) setAxis({1, 0, 0}, "X");
        if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) setAxis({0, 1, 0}, "Y");
        if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) setAxis({0, 0, 1}, "Z");
        applyModal(*o);

        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            cancelModal(*o);
        } else if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) ||
                   (overView && ImGui::IsMouseClicked(ImGuiMouseButton_Left))) {
            g_modal = Modal::None;
            sc.status = "Done";
        }
        return;
    }

    // ---- keyboard shortcuts (not while typing in the code editor or flying) ----
    if (!io.WantTextInput && !rmb) {
        if (ImGui::IsKeyPressed(ImGuiKey_Tab, false)) toggleEditMode();
        if (ImGui::IsKeyPressed(ImGuiKey_F, false)) focusSelected();
        if (ImGui::IsKeyPressed(ImGuiKey_Home, false)) resetCamera();

        if (sc.editMode) {
            if (ImGui::IsKeyPressed(ImGuiKey_1, false)) setSelectMode(SelectMode::Vertex);
            if (ImGui::IsKeyPressed(ImGuiKey_2, false)) setSelectMode(SelectMode::Face);
            if (ImGui::IsKeyPressed(ImGuiKey_A, false)) {
                if (io.KeyAlt) deselectAll(); else selectAll();
            }
            if (!io.KeyAlt && !io.KeyCtrl) {
                if (ImGui::IsKeyPressed(ImGuiKey_E, false)) startExtrude();
                if (ImGui::IsKeyPressed(ImGuiKey_G, false)) startGrab();
                if (ImGui::IsKeyPressed(ImGuiKey_S, false)) startScale();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_X, false) || ImGui::IsKeyPressed(ImGuiKey_Delete, false))
                deleteSelected();
        } else if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            deleteSelected();
        }
    }

    // ---- clicking in the 3D view ----
    if (overView && !g_navDrag && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (sc.editMode) pickInEdit(io.KeyShift);
        else pickObject();
    }
}

} // namespace Viewport