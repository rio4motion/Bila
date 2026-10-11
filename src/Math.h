#pragma once
#include <cmath>

// A point or direction in 3D space.
struct Vec3 {
    float x = 0, y = 0, z = 0;
};

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator*(float s, Vec3 a) { return a * s; }
inline Vec3 operator/(Vec3 a, float s) { return {a.x / s, a.y / s, a.z / s}; }
inline Vec3& operator+=(Vec3& a, Vec3 b) { a = a + b; return a; }

inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline float length(Vec3 a) { return std::sqrt(dot(a, a)); }
inline Vec3 normalize(Vec3 a) {
    float l = length(a);
    return l < 1e-8f ? Vec3{0, 1, 0} : a / l;
}

// A 4x4 matrix, stored column by column (the layout OpenGL expects).
struct Mat4 {
    float m[16];

    static Mat4 identity() {
        Mat4 r;
        for (int i = 0; i < 16; i++) r.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        return r;
    }
    static Mat4 translate(Vec3 t) {
        Mat4 r = identity();
        r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
        return r;
    }
    static Mat4 scale(Vec3 s) {
        Mat4 r = identity();
        r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z;
        return r;
    }
    static Mat4 rotateX(float deg) {
        float a = deg * 3.14159265f / 180.0f, c = std::cos(a), s = std::sin(a);
        Mat4 r = identity();
        r.m[5] = c; r.m[6] = s; r.m[9] = -s; r.m[10] = c;
        return r;
    }
    static Mat4 rotateY(float deg) {
        float a = deg * 3.14159265f / 180.0f, c = std::cos(a), s = std::sin(a);
        Mat4 r = identity();
        r.m[0] = c; r.m[2] = -s; r.m[8] = s; r.m[10] = c;
        return r;
    }
    static Mat4 rotateZ(float deg) {
        float a = deg * 3.14159265f / 180.0f, c = std::cos(a), s = std::sin(a);
        Mat4 r = identity();
        r.m[0] = c; r.m[1] = s; r.m[4] = -s; r.m[5] = c;
        return r;
    }
    static Mat4 perspective(float fovDeg, float aspect, float nearZ, float farZ) {
        float f = 1.0f / std::tan(fovDeg * 0.5f * 3.14159265f / 180.0f);
        Mat4 r;
        for (int i = 0; i < 16; i++) r.m[i] = 0.0f;
        r.m[0] = f / aspect;
        r.m[5] = f;
        r.m[10] = (farZ + nearZ) / (nearZ - farZ);
        r.m[11] = -1.0f;
        r.m[14] = 2.0f * farZ * nearZ / (nearZ - farZ);
        return r;
    }
    static Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
        Vec3 f = normalize(target - eye);
        Vec3 s = normalize(cross(f, up));
        Vec3 u = cross(s, f);
        Mat4 r = identity();
        r.m[0] = s.x; r.m[4] = s.y; r.m[8]  = s.z;
        r.m[1] = u.x; r.m[5] = u.y; r.m[9]  = u.z;
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
        r.m[12] = -dot(s, eye);
        r.m[13] = -dot(u, eye);
        r.m[14] = dot(f, eye);
        return r;
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 r;
        for (int j = 0; j < 4; j++)
            for (int i = 0; i < 4; i++) {
                float sum = 0;
                for (int k = 0; k < 4; k++) sum += m[k * 4 + i] * b.m[j * 4 + k];
                r.m[j * 4 + i] = sum;
            }
        return r;
    }

    // Move a point through the matrix (w is assumed to be 1).
    Vec3 point(Vec3 p) const {
        return {m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12],
                m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
                m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]};
    }
    // Same but for a direction (ignores the move part).
    Vec3 dir(Vec3 d) const {
        return {m[0] * d.x + m[4] * d.y + m[8] * d.z,
                m[1] * d.x + m[5] * d.y + m[9] * d.z,
                m[2] * d.x + m[6] * d.y + m[10] * d.z};
    }
    // Full result including w, used for turning 3D points into screen points.
    void clip(Vec3 p, float out[4]) const {
        for (int i = 0; i < 4; i++)
            out[i] = m[i] * p.x + m[4 + i] * p.y + m[8 + i] * p.z + m[12 + i];
    }
};