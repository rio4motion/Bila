cmake_minimum_required(VERSION 3.16)
project(Bila LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9
)
FetchContent_MakeAvailable(imgui)

add_library(imgui_lib STATIC
${imgui_SOURCE_DIR}/imgui.cpp
${imgui_SOURCE_DIR}/imgui_draw.cpp
${imgui_SOURCE_DIR}/imgui_tables.cpp
${imgui_SOURCE_DIR}/imgui_widgets.cpp
${imgui_SOURCE_DIR}/imgui_demo.cpp
${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

# imnodes = the node editor library. SOURCE_SUBDIR stops CMake from using its own build file.
FetchContent_Declare(
    imnodes
    GIT_REPOSITORY https://github.com/Nelarius/imnodes.git
    GIT_TAG v0.5
    SOURCE_SUBDIR do_not_build
)
FetchContent_MakeAvailable(imnodes)

add_library(imnodes_lib STATIC ${imnodes_SOURCE_DIR}/imnodes.cpp)
target_include_directories(imnodes_lib PUBLIC ${imnodes_SOURCE_DIR})
target_link_libraries(imnodes_lib PUBLIC imgui_lib)
target_compile_definitions(imnodes_lib PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

add_executable(Bila
    src/main.cpp
    src/Viewport.cpp
    src/renderer/Rendering.cpp
    UI/ImGuiLayer.cpp
)
target_include_directories(Bila PRIVATE src UI)
target_link_libraries(Bila PRIVATE glfw OpenGL::GL imgui_lib imnodes_lib)