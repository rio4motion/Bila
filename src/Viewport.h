#pragma once
#include "Scene.h"

// Everything that happens when you click, drag or press keys in the 3D view.
namespace Viewport {
    void update();                       // call once per frame, after ImGuiLayer::beginFrame()

    void resetCamera();
    void focusSelected();

    void toggleEditMode();               // Tab
    void setSelectMode(SelectMode mode); // 1 = vertex, 2 = face
    void selectAll();                    // A
    void deselectAll();                  // Alt + A
    void startExtrude();                 // E
    void startGrab();                    // G
    void startScale();                   // S
    void deleteSelected();               // X or Delete
    bool modalActive();                  // true while moving / scaling
}