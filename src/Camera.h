#pragma once
#include "Math.h"
#include <algorithm>

// An orbit camera: it circles around a target point, like Blender's viewport.
struct Camera {
    Vec3 target{0.0f, 0.5f, 0.0f};
    float distance = 8.0f;   // how far from the target
    float yaw = 35.0f;       // left/right angle in degrees
    float pitch = 25.0f;     // up/down angle in degrees
    float fovY = 60.0f;

    Vec3 position() const {
        float y = yaw * 3.14159265f / 180.0f;
        float p = pitch * 3.14159265f / 180.0f;
        return target + Vec3{distance * std::cos(p) * std::sin(y),
                             distance * std::sin(p),
                             distance * std::cos(p) * std::cos(y)};
    }
    Vec3 forward() const { return normalize(target - position()); }
    Vec3 right() const { return normalize(cross(forward(), Vec3{0, 1, 0})); }
    Vec3 up() const { return cross(right(), forward()); }

    Mat4 view() const { return Mat4::lookAt(position(), target, Vec3{0, 1, 0}); }
    Mat4 proj(float aspect) const { return Mat4::perspective(fovY, aspect, 0.05f, 300.0f); }

    void orbit(float dx, float dy) {
        yaw -= dx * 0.4f;
        pitch += dy * 0.4f;
        pitch = std::max(-89.0f, std::min(89.0f, pitch));
    }
    // dx, dy are already in world units
    void pan(float dx, float dy) { target = target - right() * dx + up() * dy; }
    void zoom(float scroll) {
        distance *= std::pow(0.88f, scroll);
        distance = std::max(0.3f, std::min(150.0f, distance));
    }
};