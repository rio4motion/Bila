#pragma once
#include "Math.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

typedef std::vector<int> Face;   // a face is a list of vertex numbers, in order

// An editable mesh: a list of points (v) and a list of faces (f) that join them.
// vsel / fsel remember which vertices / faces are selected in edit mode.
struct Mesh {
    std::vector<Vec3> v;
    std::vector<Face> f;
    std::vector<char> vsel;
    std::vector<char> fsel;

    static constexpr float kPi = 3.14159265f;

    // ---------- basics ----------
    static Vec3 normalOf(const std::vector<Vec3>& verts, const Face& face) {
        Vec3 n;
        int cnt = (int)face.size();
        for (int i = 0; i < cnt; i++) {
            const Vec3& a = verts[face[i]];
            const Vec3& b = verts[face[(i + 1) % cnt]];
            n.x += (a.y - b.y) * (a.z + b.z);
            n.y += (a.z - b.z) * (a.x + b.x);
            n.z += (a.x - b.x) * (a.y + b.y);
        }
        return normalize(n);
    }
    Vec3 faceNormal(int i) const { return normalOf(v, f[i]); }
    Vec3 faceCenter(int i) const {
        Vec3 c;
        for (int id : f[i]) c += v[id];
        return c / (float)f[i].size();
    }
    // How many triangles the renderer needs (a face with n points = n-2 triangles).
    int triangleCount() const {
        int t = 0;
        for (const Face& face : f) if (face.size() >= 3) t += (int)face.size() - 2;
        return t;
    }

    // Adds a face and flips it if needed so it points away from "awayFrom".
    void addFace(Face face, Vec3 awayFrom) {
        Vec3 c;
        for (int id : face) c += v[id];
        c = c / (float)face.size();
        if (dot(normalOf(v, face), c - awayFrom) < 0) std::reverse(face.begin(), face.end());
        f.push_back(std::move(face));
    }

    // ---------- selection ----------
    void resetSelection() {
        vsel.assign(v.size(), 0);
        fsel.assign(f.size(), 0);
    }
    void fixSelectionSize() {
        vsel.resize(v.size(), 0);
        fsel.resize(f.size(), 0);
    }
    // A face counts as selected when all of its points are selected.
    void syncFacesFromVerts() {
        fixSelectionSize();
        for (size_t i = 0; i < f.size(); i++) {
            bool all = true;
            for (int id : f[i]) if (!vsel[id]) { all = false; break; }
            fsel[i] = all ? 1 : 0;
        }
    }
    // The points of selected faces become selected.
    void syncVertsFromFaces() {
        fixSelectionSize();
        std::fill(vsel.begin(), vsel.end(), 0);
        for (size_t i = 0; i < f.size(); i++)
            if (fsel[i]) for (int id : f[i]) vsel[id] = 1;
    }
    bool anyVertSelected() const {
        for (char c : vsel) if (c) return true;
        return false;
    }

    // ---------- editing ----------
    // Extrude: copy the selected faces, pull them away and connect them to the
    // old shape with new side faces. Returns false if no face was selected.
    bool extrudeFaces(Vec3& averageNormal) {
        fixSelectionSize();
        std::vector<int> sel;
        for (size_t i = 0; i < f.size(); i++) if (fsel[i]) sel.push_back((int)i);
        if (sel.empty()) return false;

        Vec3 n;
        for (int fi : sel) n += faceNormal(fi);
        averageNormal = normalize(n);

        // An edge on the border of the selection is used by only one selected face.
        std::map<std::pair<int, int>, int> edgeCount;
        for (int fi : sel) {
            const Face& face = f[fi];
            for (size_t k = 0; k < face.size(); k++) {
                int a = face[k], b = face[(k + 1) % face.size()];
                edgeCount[{std::min(a, b), std::max(a, b)}]++;
            }
        }

        // Copy every point used by the selected faces.
        std::map<int, int> copyOf;
        for (int fi : sel)
            for (int id : f[fi])
                if (!copyOf.count(id)) {
                    copyOf[id] = (int)v.size();
                    v.push_back(v[id]);
                }

        // Build the side faces along the border.
        std::vector<Face> sides;
        for (int fi : sel) {
            const Face& face = f[fi];
            for (size_t k = 0; k < face.size(); k++) {
                int a = face[k], b = face[(k + 1) % face.size()];
                if (edgeCount[{std::min(a, b), std::max(a, b)}] == 1)
                    sides.push_back({a, b, copyOf[b], copyOf[a]});
            }
        }

        // The selected faces now use the copied points.
        for (int fi : sel)
            for (int& id : f[fi]) id = copyOf[id];
        for (const Face& s : sides) f.push_back(s);

        fixSelectionSize();
        syncVertsFromFaces();
        return true;
    }

    void removeFaces(const std::vector<char>& kill) {
        std::vector<Face> keep;
        keep.reserve(f.size());
        for (size_t i = 0; i < f.size(); i++)
            if (!kill[i]) keep.push_back(std::move(f[i]));
        f = std::move(keep);

        // throw away points that no face uses any more
        std::vector<int> remap(v.size(), -1);
        std::vector<Vec3> nv;
        nv.reserve(v.size());
        for (Face& face : f)
            for (int& id : face) {
                if (remap[id] < 0) { remap[id] = (int)nv.size(); nv.push_back(v[id]); }
                id = remap[id];
            }
        v = std::move(nv);
        resetSelection();
    }
    // faceMode: delete selected faces. Otherwise delete every face touching a selected point.
    void deleteSelected(bool faceMode) {
        fixSelectionSize();
        std::vector<char> kill(f.size(), 0);
        for (size_t i = 0; i < f.size(); i++) {
            if (faceMode) kill[i] = fsel[i];
            else for (int id : f[i]) if (vsel[id]) { kill[i] = 1; break; }
        }
        removeFaces(kill);
    }

    // ---------- primitives ----------
    static Mesh cube() {
        Mesh m;
        const float h = 0.5f;
        m.v = {{-h,-h,-h}, {h,-h,-h}, {h,h,-h}, {-h,h,-h}, {-h,-h,h}, {h,-h,h}, {h,h,h}, {-h,h,h}};
        const int q[6][4] = {{4,5,6,7}, {1,0,3,2}, {0,4,7,3}, {5,1,2,6}, {3,7,6,2}, {0,1,5,4}};
        for (auto& f4 : q) m.addFace({f4[0], f4[1], f4[2], f4[3]}, Vec3{});
        m.resetSelection();
        return m;
    }

    static Mesh plane() {
        Mesh m;
        m.v = {{-0.5f, 0, -0.5f}, {-0.5f, 0, 0.5f}, {0.5f, 0, 0.5f}, {0.5f, 0, -0.5f}};
        m.addFace({0, 1, 2, 3}, Vec3{0, -1, 0});
        m.resetSelection();
        return m;
    }

    // A flat sheet cut into n x n squares
    static Mesh grid(int n = 4) {
        Mesh m;
        m.v.reserve((size_t)(n + 1) * (n + 1));
        m.f.reserve((size_t)n * n);
        for (int j = 0; j <= n; j++)
            for (int i = 0; i <= n; i++)
                m.v.push_back({-0.5f + (float)i / n, 0.0f, -0.5f + (float)j / n});
        auto id = [n](int i, int j) { return j * (n + 1) + i; };
        for (int j = 0; j < n; j++)
            for (int i = 0; i < n; i++)
                m.addFace({id(i, j), id(i, j + 1), id(i + 1, j + 1), id(i + 1, j)}, Vec3{0, -1, 0});
        m.resetSelection();
        return m;
    }

    static Mesh sphere(int segs = 16, int rings = 10) {
        Mesh m;
        m.v.reserve((size_t)segs * (rings - 1) + 2);
        m.f.reserve((size_t)segs * rings);
        m.v.push_back({0, 0.5f, 0});    // 0 = top
        m.v.push_back({0, -0.5f, 0});   // 1 = bottom
        for (int r = 1; r < rings; r++) {
            float phi = kPi * (float)r / rings;
            for (int s = 0; s < segs; s++) {
                float th = 2 * kPi * (float)s / segs;
                m.v.push_back({0.5f * std::sin(phi) * std::cos(th), 0.5f * std::cos(phi),
                               0.5f * std::sin(phi) * std::sin(th)});
            }
        }
        auto id = [segs](int r, int s) { return 2 + (r - 1) * segs + (s % segs); };
        for (int s = 0; s < segs; s++) m.addFace({0, id(1, s), id(1, s + 1)}, Vec3{});
        for (int r = 1; r < rings - 1; r++)
            for (int s = 0; s < segs; s++)
                m.addFace({id(r, s), id(r, s + 1), id(r + 1, s + 1), id(r + 1, s)}, Vec3{});
        for (int s = 0; s < segs; s++) m.addFace({1, id(rings - 1, s + 1), id(rings - 1, s)}, Vec3{});
        m.resetSelection();
        return m;
    }

    static Mesh cylinder(int n = 16) {
        Mesh m;
        for (int i = 0; i < n; i++) {
            float a = 2 * kPi * (float)i / n;
            m.v.push_back({0.5f * std::cos(a), -0.5f, 0.5f * std::sin(a)});
        }
        for (int i = 0; i < n; i++) {
            float a = 2 * kPi * (float)i / n;
            m.v.push_back({0.5f * std::cos(a), 0.5f, 0.5f * std::sin(a)});
        }
        for (int i = 0; i < n; i++)
            m.addFace({i, (i + 1) % n, n + (i + 1) % n, n + i}, Vec3{});
        Face bottom, top;
        for (int i = 0; i < n; i++) { bottom.push_back(i); top.push_back(n + i); }
        m.addFace(bottom, Vec3{});
        m.addFace(top, Vec3{});
        m.resetSelection();
        return m;
    }

    static Mesh cone(int n = 16) {
        Mesh m;
        for (int i = 0; i < n; i++) {
            float a = 2 * kPi * (float)i / n;
            m.v.push_back({0.5f * std::cos(a), -0.5f, 0.5f * std::sin(a)});
        }
        m.v.push_back({0, 0.5f, 0});   // tip
        for (int i = 0; i < n; i++)
            m.addFace({i, (i + 1) % n, n}, Vec3{0, -1.0f / 6.0f, 0});
        Face base;
        for (int i = 0; i < n; i++) base.push_back(i);
        m.addFace(base, Vec3{});
        m.resetSelection();
        return m;
    }

    static Mesh torus(int bigSegs = 24, int smallSegs = 12) {
        Mesh m;
        const float R = 0.35f, r = 0.15f;
        for (int i = 0; i < bigSegs; i++) {
            float th = 2 * kPi * (float)i / bigSegs;
            for (int j = 0; j < smallSegs; j++) {
                float ph = 2 * kPi * (float)j / smallSegs;
                float d = R + r * std::cos(ph);
                m.v.push_back({d * std::cos(th), r * std::sin(ph), d * std::sin(th)});
            }
        }
        auto id = [=](int i, int j) { return (i % bigSegs) * smallSegs + (j % smallSegs); };
        for (int i = 0; i < bigSegs; i++) {
            float thm = 2 * kPi * ((float)i + 0.5f) / bigSegs;
            Vec3 tubeCenter{R * std::cos(thm), 0, R * std::sin(thm)};
            for (int j = 0; j < smallSegs; j++)
                m.addFace({id(i, j), id(i + 1, j), id(i + 1, j + 1), id(i, j + 1)}, tubeCenter);
        }
        m.resetSelection();
        return m;
    }
};