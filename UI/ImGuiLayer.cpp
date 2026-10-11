#include "ImGuiLayer.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace ImGuiLayer {
    struct SceneObject {
        const char* name;
        float position[3];
        float rotation[3];
        float scale[3];
    };

    static SceneObject objects[] = {
        { "Cube",   {0, 0, 0}, {0, 0, 0}, {1, 1, 1} },
        { "Sphere", {2, 0, 0}, {0, 0, 0}, {1, 1, 1} },
        { "Camera", {0, 2, 5}, {0, 0, 0}, {1, 1, 1} },
        { "Light",  {0, 5, 0}, {0, 0, 0}, {1, 1, 1} },
    };
    static int selected = 0;

    // The text typed in the Code panel (a "char array" holds text, 4096 letters max)
    static char code[4096] = "// Write Bila C++ here\n";

    void init(GLFWwindow* window) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
    }

    void beginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Top menu bar
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
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Wireframe");
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGuiViewport* vp = ImGui::GetMainViewport();
        float panelW = 250.0f;
        float codeH = 220.0f;
        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

        // Left panel: object list
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(panelW, vp->WorkSize.y));
        ImGui::Begin("Scene", nullptr, flags);
        for (int i = 0; i < 4; i++) {
            if (ImGui::Selectable(objects[i].name, selected == i))
                selected = i;
        }
        ImGui::End();

        // Right panel: properties of the selected object
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - panelW, vp->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(panelW, vp->WorkSize.y));
        ImGui::Begin("Properties", nullptr, flags);
        SceneObject& obj = objects[selected];
        ImGui::Text("Selected: %s", obj.name);
        ImGui::Separator();
        ImGui::DragFloat3("Position", obj.position, 0.1f);
        ImGui::DragFloat3("Rotation", obj.rotation, 1.0f);
        ImGui::DragFloat3("Scale", obj.scale, 0.05f);
        ImGui::End();

        // Bottom panel: code editor (sits between the left and right panels)
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + panelW, vp->WorkPos.y + vp->WorkSize.y - codeH));
        ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x - 2 * panelW, codeH));
        ImGui::Begin("Code", nullptr, flags);
        if (ImGui::Button("Run")) {
            // Does nothing yet
        }
        ImGui::InputTextMultiline("##code", code, sizeof(code),
            ImVec2(-1, -1), ImGuiInputTextFlags_AllowTabInput);
        ImGui::End();
    }

    void endFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}