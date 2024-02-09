#pragma once

#include "imgui.h"
#include <igl/opengl/glfw/Viewer.h>
#include <igl/opengl/glfw/ViewerPlugin.h>
#include <spdlog/spdlog.h>

#include "ImGuiContext/ImGuiContext.hpp"
#include "ImGuiContext/OpenGLFrameBuffer.hpp"

#include "Util/Profiler.hpp"

#include <memory>
#include <string>

extern Util::Profiler g_FrameProfiler;
extern Util::Profiler g_StepProfiler;
extern Util::Profiler g_PreComputeProfiler;

namespace FS
{

    struct FSViewer : public igl::opengl::glfw::ViewerPlugin
    {
        std::unique_ptr<OpenGLFrameBuffer> m_frameBuffer;
        std::unique_ptr<ImGuiContext>      m_imguiContext;

        Eigen::MatrixXd m_V;
        Eigen::MatrixXi m_F;

        int m_clickVert = -1;

        bool                m_isSceneInterationActive = true;
        std::pair<int, int> m_sceneWindowSize         = {1280, 800};
        ImVec2              m_sceneWindowPos;
        ImVec2              m_sceneCursorPos;

        bool m_isSingleStep = false;

    public:
        FSViewer()  = default;
        ~FSViewer() = default;

        void Setup() {}
        void Reset() {}

    public:
        void init(igl::opengl::glfw::Viewer* _viewer) override;
        void shutdown() override { m_imguiContext->end(); }

        bool pre_draw() override;

    public:
        void ExportPNG(std::string url = "./Default_FSViewer_Export.png");

    public:
        bool mouse_down(int button, int modifier) override;
        bool mouse_move(int /*mouse_x*/, int /*mouse_y*/) override { return !m_isSceneInterationActive; }
        bool mouse_up(int /*button*/, int /*modifier*/) override { return !m_isSceneInterationActive; }
        bool mouse_scroll(float /*delta_y*/) override { return !m_isSceneInterationActive; }

        bool key_pressed(unsigned int /*key*/, int /*modifiers*/) override { return !m_isSceneInterationActive; }
        bool key_down(int /*key*/, int /*modifiers*/) override { return !m_isSceneInterationActive; }
        bool key_up(int /*key*/, int /*modifiers*/) override { return !m_isSceneInterationActive; }

    private:
        void SimulationInfoWindow();
        void OperationWindow();
        void ProfilerWindow();
    };
}; // namespace FS

// NOLINTBEGIN(*)
// NOLINTEND(*)