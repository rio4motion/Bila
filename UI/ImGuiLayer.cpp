#include "ImGuiLayer.h"
#include "Scene.h"
#include "Viewport.h"
#include "imgui.h"
#include "imnodes.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <vector>
#include <utility>

namespace ImGuiLayer {
    static char code[4096] = "// Write Bila C++ here\n";

    // Blueprint editor data
    static bool showBlueprint = false;
    static std::vector<std::pair<int, int>> links;

    struct PrimitiveInfo { const char* label; ObjectType type; };
    static const PrimitiveInfo primitives[] = {
        { "Cube", ObjectType::Cube },         { "Sphere", ObjectType::Sphere },
        { "Plane", ObjectType::Plane },       { "Grid", ObjectType::Grid },
        { "Cylinder", ObjectType::Cylinder }, { "Cone", ObjectType::Cone },
        { "Torus", ObjectType::Torus },
    };
    static const char* renderModeNames[] = {
        "Solid", "Wireframe", "Solid + Wireframe", "Points", "Normals", "Unlit"
    };

    void init(GLFWwindow* window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImNodes::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
    }

    static void drawBlueprintEditor() {
        if (!showBlueprint) return;

        ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Blueprint Editor", &showBlueprint);

        static bool firstTime = true;
        static float number = 5.0f;

        ImNodes::BeginNodeEditor();

        // Node 1: On Start
        ImNodes::BeginNode(1);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("On Start");
        ImNodes::EndNodeTitleBar();
        ImNodes::BeginOutputAttribute(11);
        ImGui::Text("Run >");
        ImNodes::EndOutputAttribute();
        ImNodes::EndNode();

        // Node 2: Number
        ImNodes::BeginNode(2);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Number");
        ImNodes::EndNodeTitleBar();
        ImNodes::BeginOutputAttribute(21);
        ImGui::PushItemWidth(80);
        ImGui::DragFloat("##number", &number, 0.1f);
        ImGui::PopItemWidth();
        ImNodes::EndOutputAttribute();
        ImNodes::EndNode();

        // Node 3: Set Position
        ImNodes::BeginNode(3);
        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Set Position");
        ImNodes::EndNodeTitleBar();
        ImNodes::BeginInputAttribute(31);
        ImGui::Text("> Start");
        ImNodes::EndInputAttribute();
        ImNodes::BeginInputAttribute(32);
        ImGui::Text("X value");
        ImNodes::EndInputAttribute();
        ImNodes::EndNode();

        for (int i = 0; i < (int)links.size(); i++)
            ImNodes::Link(i, links[i].first, links[i].second);

        ImNodes::EndNodeEditor();

        if (firstTime) {
            ImNodes::SetNodeGridSpacePos(1, ImVec2(20, 40));
            ImNodes::SetNodeGridSpacePos(2, ImVec2(20, 160));
            ImNodes::SetNodeGridSpacePos(3, ImVec2(300, 80));
            firstTime = false;
        }

        int startId, endId;
        if (ImNodes::IsLinkCreated(&startId, &endId))
            links.push_back({ startId, endId });

        ImGui::End();
    }

    // The tools shown in the left panel while you are in Edit Mode
    static void drawEditTools(Scene& scene) {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.1f, 1.0f), "EDIT MODE");

        int mode = (scene.selectMode == SelectMode::Vertex) ? 0 : 1;
        if (ImGui::RadioButton("Points (1)", mode == 0)) Viewport::setSelectMode(SelectMode::Vertex);
        ImGui::SameLine();
        if (ImGui::RadioButton("Faces (2)", mode == 1)) Viewport::setSelectMode(SelectMode::Face);

        ImVec2 half(112, 0);
        if (ImGui::Button("Select All", half)) Viewport::selectAll();
        ImGui::SameLine();
        if (ImGui::Button("Deselect", half)) Viewport::deselectAll();
        if (ImGui::Button("Extrude (E)", half)) Viewport::startExtrude();
        ImGui::SameLine();
        if (ImGui::Button("Delete (X)", half)) Viewport::deleteSelected();
        if (ImGui::Button("Move (G)", half)) Viewport::startGrab();
        ImGui::SameLine();
        if (ImGui::Button("Scale (S)", half)) Viewport::startScale();

        if (ImGui::Button("Back to Object Mode (Tab)", ImVec2(-1, 0))) Viewport::toggleEditMode();
    }

    void beginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        Scene& scene = getScene();

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::MenuItem("New");
                ImGui::MenuItem("Open");
                ImGui::MenuItem("Save");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                ImGui::MenuItem("Undo");
                ImGui::MenuItem("Redo");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Add")) {
                for (const PrimitiveInfo& p : primitives)
                    if (ImGui::MenuItem(p.label)) scene.add(p.type, p.label);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                for (int i = 0; i < 6; i++)
                    if (ImGui::MenuItem(renderModeNames[i], nullptr, (int)scene.renderMode == i))
                        scene.renderMode = (RenderMode)i;
                ImGui::Separator();
                if (ImGui::MenuItem("Reset Camera (Home)")) Viewport::resetCamera();
                if (ImGui::MenuItem("Focus Selected (F)")) Viewport::focusSelected();
                ImGui::Separator();
                ImGui::MenuItem("Blueprint Editor", nullptr, &showBlueprint);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGuiViewport* vp = ImGui::GetMainViewport();
        float panelW = 250.0f;
        float codeH = 220.0f;
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

        // Left panel: tools + object list
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(panelW, vp->WorkSize.y));
        ImGui::Begin("Scene", nullptr, flags);

        int shading = (int)scene.renderMode;
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##shading", &shading, renderModeNames, 6)) scene.renderMode = (RenderMode)shading;

        ImGui::Separator();
        if (scene.editMode) {
            drawEditTools(scene);
        } else {
            ImGui::Text("Add:");
            ImVec2 half(112, 0);
            for (int i = 0; i < (int)(sizeof(primitives) / sizeof(primitives[0])); i++) {
                if (i % 2 == 1) ImGui::SameLine();
                if (ImGui::Button(primitives[i].label, half))
                    scene.add(primitives[i].type, primitives[i].label);
            }
            if (ImGui::Button("Edit Mode (Tab)", half)) Viewport::toggleEditMode();
            ImGui::SameLine();
            if (ImGui::Button("Delete", half)) scene.removeSelected();
        }

        ImGui::Separator();
        ImGui::TextWrapped("%s", scene.status.c_str());
        ImGui::Separator();

        for (int i = 0; i < (int)scene.objects.size(); i++) {
            ImGui::PushID(i);
            if (ImGui::Selectable(scene.objects[i].name.c_str(), scene.selected == i)) {
                if (scene.selected != i) scene.editMode = false;
                scene.selected = i;
            }
            ImGui::PopID();
        }

        ImGui::Separator();
        if (ImGui::CollapsingHeader("Controls")) {
            ImGui::TextWrapped(
                "Right mouse drag: orbit\n"
                "Middle mouse drag: orbit\n"
                "Shift + drag: pan\n"
                "Mouse wheel: zoom\n"
                "Hold right mouse + W A S D Q E: fly\n"
                "F: focus on selected\n"
                "Home: reset camera\n"
                "Click: select\n"
                "Shift + click: add to selection\n"
                "Tab: edit mode\n"
                "1 / 2: points / faces\n"
                "A / Alt+A: select all / none\n"
                "E: extrude, G: move, S: scale\n"
                "X / Y / Z while moving: lock axis\n"
                "X or Delete: delete");
        }
        ImGui::End();

        // Right panel: properties
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - panelW, vp->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(panelW, vp->WorkSize.y));
        ImGui::Begin("Properties", nullptr, flags);
        SceneObject* obj = scene.selectedObject();
        if (obj) {
            ImGui::Text("Selected: %s", obj->name.c_str());
            ImGui::Separator();
            ImGui::DragFloat3("Position", obj->position, 0.1f);
            ImGui::DragFloat3("Rotation", obj->rotation, 1.0f);
            ImGui::DragFloat3("Scale", obj->scale, 0.05f);
            if (isMeshType(obj->type)) {
                ImGui::ColorEdit3("Color", obj->color);
                ImGui::Separator();
                ImGui::Text("Points: %d", (int)obj->mesh.v.size());
                ImGui::Text("Faces: %d", (int)obj->mesh.f.size());
            }
        } else {
            ImGui::Text("Nothing selected");
        }
        ImGui::End();

        // Bottom panel: code editor
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + panelW, vp->WorkPos.y + vp->WorkSize.y - codeH));
        ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x - 2 * panelW, codeH));
        ImGui::Begin("Code", nullptr, flags);
        if (ImGui::Button("Run")) {
            // Does nothing yet
        }
        ImGui::InputTextMultiline("##code", code, sizeof(code),
            ImVec2(-1, -1), ImGuiInputTextFlags_AllowTabInput);
        ImGui::End();

        drawBlueprintEditor();
    }

    void endFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImNodes::DestroyContext();
        ImGui::DestroyContext();
    }
}