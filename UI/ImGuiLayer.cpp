#include "ImGuiLayer.h"
#include "imgui.h"
#include "imnodes.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <vector>
#include <utility>

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
    static char code[4096] = "// Write Bila C++ here\n";

    // Blueprint editor data
    static bool showBlueprint = false;
    static std::vector<std::pair<int, int>> links;

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

    void beginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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

        // Left panel: object list
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(panelW, vp->WorkSize.y));
        ImGui::Begin("Scene", nullptr, flags);
        for (int i = 0; i < 4; i++) {
            if (ImGui::Selectable(objects[i].name, selected == i))
                selected = i;
        }
        ImGui::End();

        // Right panel: properties
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