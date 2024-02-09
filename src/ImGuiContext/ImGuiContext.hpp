#pragma once

#include "GLFW/glfw3.h"
#include "imgui.h"

namespace FS
{
    struct ImGuiContext
    {
        ImVec2 root_window_pos;

        bool init(GLFWwindow* window);
        void preRender();
        void postRender();
        void end();
    };
} // namespace FS