0

#include "ImGuiLayer.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace ImGuiLayer {
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

        // Right side panel (fixed in place)
        ImGuiViewport* vp = ImGui::GetMainViewport();
        float panelW = 300.0f;
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x - panelW, vp->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(panelW, vp->WorkSize.y));
        ImGui::Begin("Properties", nullptr,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::Text("Object properties go here");
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