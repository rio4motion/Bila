#pragma once
#include "Camera.h"
#include "Mesh.h"
#include <string>
#include <vector>

enum class ObjectType { Cube, Sphere, Plane, Grid, Cylinder, Cone, Torus, Camera, Light };
enum class RenderMode { Solid, Wireframe, SolidWire, Points, Normals, Unlit };
enum class SelectMode { Vertex, Face };

inline bool isMeshType(ObjectType t) { return t != ObjectType::Camera && t != ObjectType::Light; }

struct SceneObject {
    std::string name;
    ObjectType type = ObjectType::Cube;
    float position[3] = {0, 0, 0};
    float rotation[3] = {0, 0, 0};
    float scale[3] = {1, 1, 1};
    float color[3] = {0.8f, 0.8f, 0.8f};
    Mesh mesh;   // empty for Camera and Light

    // Where the object sits in the world: move * rotate * scale
    Mat4 model() const {
        return Mat4::translate({position[0], position[1], position[2]}) *
               Mat4::rotateX(rotation[0]) * Mat4::rotateY(rotation[1]) * Mat4::rotateZ(rotation[2]) *
               Mat4::scale({scale[0], scale[1], scale[2]});
    }
    // The opposite of model(): world space back to the object's own space
    Mat4 invModel() const {
        auto safe = [](float s) { return std::fabs(s) < 1e-4f ? 1e-4f : s; };
        return Mat4::scale({1 / safe(scale[0]), 1 / safe(scale[1]), 1 / safe(scale[2])}) *
               Mat4::rotateZ(-rotation[2]) * Mat4::rotateY(-rotation[1]) * Mat4::rotateX(-rotation[0]) *
               Mat4::translate({-position[0], -position[1], -position[2]});
    }
};

inline Mesh makeMesh(ObjectType t) {
    switch (t) {
        case ObjectType::Cube:     return Mesh::cube();
        case ObjectType::Sphere:   return Mesh::sphere();
        case ObjectType::Plane:    return Mesh::plane();
        case ObjectType::Grid:     return Mesh::grid();
        case ObjectType::Cylinder: return Mesh::cylinder();
        case ObjectType::Cone:     return Mesh::cone();
        case ObjectType::Torus:    return Mesh::torus();
        default:                   return Mesh();
    }
}

struct Scene {
    std::vector<SceneObject> objects;
    int selected = -1;
    int counter = 0;

    bool editMode = false;
    SelectMode selectMode = SelectMode::Vertex;
    RenderMode renderMode = RenderMode::Solid;
    std::string status = "Ready";

    Scene() {
        add(ObjectType::Cube, "Cube");
        add(ObjectType::Camera, "Camera");
        add(ObjectType::Light, "Light");
        selected = 0;
    }

    void add(ObjectType type, const std::string& baseName) {
        SceneObject o;
        o.type = type;
        o.name = baseName + " " + std::to_string(++counter);
        o.mesh = makeMesh(type);
        static const float colors[][3] = {
            {0.95f, 0.55f, 0.20f}, {0.30f, 0.70f, 0.95f}, {0.70f, 0.70f, 0.75f},
            {0.60f, 0.75f, 0.60f}, {0.45f, 0.85f, 0.45f}, {0.75f, 0.50f, 0.90f},
            {0.95f, 0.45f, 0.65f}};
        int ci = (int)type;
        if (ci < 7) for (int k = 0; k < 3; k++) o.color[k] = colors[ci][k];
        if (type == ObjectType::Camera) { o.position[1] = 2; o.position[2] = 5; }
        if (type == ObjectType::Light)  { o.position[1] = 5; }
        objects.push_back(o);
        selected = (int)objects.size() - 1;
        editMode = false;
    }

    void removeSelected() {
        if (selected < 0 || selected >= (int)objects.size()) return;
        objects.erase(objects.begin() + selected);
        selected = -1;
        editMode = false;
    }

    SceneObject* selectedObject() {
        if (selected < 0 || selected >= (int)objects.size()) return nullptr;
        return &objects[selected];
    }
};

inline Scene& getScene() {
    static Scene scene;
    return scene;
}
inline Camera& getCamera() {
    static Camera camera;
    return camera;
}