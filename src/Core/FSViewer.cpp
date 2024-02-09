#include "FSViewer.hpp"

#include <igl/stb/write_image.h>
#include <igl/unproject_onto_mesh.h>
#include <imgui_internal.h>

namespace FS
{

    // override for `igl::opengl::glfw::ViewerPlugin` init
    void FSViewer::init(igl::opengl::glfw::Viewer* _viewer)
    {
        viewer = _viewer;

        m_imguiContext = std::make_unique<ImGuiContext>();
        m_imguiContext->init(viewer->window);

        m_frameBuffer = std::make_unique<OpenGLFrameBuffer>();
        {
            glfwGetWindowSize(viewer->window, &m_sceneWindowSize.first, &m_sceneWindowSize.second);
            spdlog::info(">>> Window m_sceneWindowSize = ({}, {})", m_sceneWindowSize.first, m_sceneWindowSize.second);
            m_frameBuffer->createBuffers(m_sceneWindowSize.first, m_sceneWindowSize.second);
        }
    }

    // Render full frame
    bool FSViewer::pre_draw()
    {
        PROFILE("FRAME");
        {
            PROFILE("PRE_DRAW");
            { // imgui context
                m_imguiContext->preRender();
                m_frameBuffer->bind();
            }

            { // update mesh data
                viewer->data().set_mesh(m_V, m_F);
                if (m_clickVert >= 0)
                {
                    auto c                    = Eigen::RowVector3d(0, 1, 0);
                    viewer->data().point_size = 20.f;
                    viewer->data().set_points(m_V.row(m_clickVert), c);
                }
            }
        }

        { // viewer core draw mesh
            PROFILE("DRAW_MESH");
            // viewer->data().dirty = 0;
            for (auto& core : viewer->core_list)
            {
                for (auto& mesh : viewer->data_list)
                {
                    if (mesh.is_visible & core.id)
                    {
                        core.draw(mesh);
                    }
                }
            }
        }

        {
            PROFILE("POST_DRAW");
            m_frameBuffer->unbind();

            // Render window
            ImVec2 viewport_panel_size;
            { // Scene window
                ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoMove);
                GLuint texture_id   = m_frameBuffer->getTexture();
                viewport_panel_size = ImGui::GetContentRegionAvail();
                m_sceneWindowPos    = ImGui::GetWindowPos();
                m_sceneCursorPos    = ImGui::GetCursorPos();
                ImGui::Image((void*)static_cast<intptr_t>(texture_id),
                             ImVec2 {viewport_panel_size.x, viewport_panel_size.y},
                             ImVec2 {0, 1},
                             ImVec2 {1, 0});
                m_isSceneInterationActive = ImGui::IsItemHovered();
                ImGui::End();
            }
            {
                SimulationInfoWindow();
                OperationWindow();
                ProfilerWindow();
            }
            {
                m_imguiContext->postRender();
                if (viewport_panel_size.x != m_sceneWindowSize.first || viewport_panel_size.y != m_sceneWindowSize.second)
                {
                    m_sceneWindowSize       = {viewport_panel_size.x, viewport_panel_size.y};
                    viewer->core().viewport = {0, 0, m_sceneWindowSize.first, m_sceneWindowSize.second};
                    m_frameBuffer->createBuffers(viewport_panel_size.x, viewport_panel_size.y);
                    spdlog::info(">>> Scene window size changed: ({}, {})", m_sceneWindowSize.first, m_sceneWindowSize.second);
                    spdlog::info(">>> viewer->core().viewport = ({}, {}, {}, {})",
                                 viewer->core().viewport(0),
                                 viewer->core().viewport(1),
                                 viewer->core().viewport(2),
                                 viewer->core().viewport(3));
                }
            }
        }

        return true;
    }

    // ======================================== Export data ========================================
    void FSViewer::ExportPNG(std::string url) // Screenshot
    {
        double scale = 1.0;
        viewer->data().point_size *= scale;

        int width  = static_cast<int>(scale * (viewer->core().viewport[2] - viewer->core().viewport[0]));
        int height = static_cast<int>(scale * (viewer->core().viewport[3] - viewer->core().viewport[1]));

        // Allocate temporary buffers for image
        Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> r(width, height);
        Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> g(width, height);
        Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> b(width, height);
        Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> a(width, height);

        // Draw the scene in the buffers
        viewer->core().draw_buffer(viewer->data(), false, r, g, b, a);

        // Save it to a PNG
        igl::stb::write_image(url, r, g, b, a);

        viewer->data().point_size /= scale;
    }

    // ======================================== User interaction ========================================
    bool FSViewer::mouse_down(int /*button*/, int modifier)
    {
        if (!m_isSceneInterationActive)
            return true;
        if (modifier != GLFW_MOD_CONTROL)
            return false;
        float x = viewer->current_mouse_x - (m_sceneWindowPos.x - m_imguiContext->root_window_pos.x + m_sceneCursorPos.x);
        float y = viewer->core().viewport(3) -
                  (viewer->current_mouse_y - (m_sceneWindowPos.y - m_imguiContext->root_window_pos.y + m_sceneCursorPos.y));
        int             fid;
        Eigen::Vector3f bc;
        bool            hit = igl::unproject_onto_mesh(
            Eigen::Vector2f(x, y), viewer->core().view, viewer->core().proj, viewer->core().viewport, m_V, m_F, fid, bc);
        if (hit)
        {
            int c;
            bc.maxCoeff(&c);
            m_clickVert = m_F(fid, c);

            return true;
        }

        return false;
    }

    // ======================================== Windows ========================================
    void FSViewer::SimulationInfoWindow()
    {
        ImGui::Begin("Simulatino Info");

        // Mesh info
        ImGui::BulletText("%s", fmt::format("Mesh V = #{}, F = #{}", m_V.rows(), m_F.rows()).c_str());

        ImGui::End();
    }

    void FSViewer::OperationWindow()
    {
        ImGui::Begin("Operation");
        float const w = ImGui::GetContentRegionAvail().x;
        float const p = 0.0f;

        { // Start and Stop Simulate
            if (!viewer->core().is_animating)
            {
                if (ImGui::Button("Start Simulate", {(w - p), 0}))
                {
                    viewer->core().is_animating = true;
                }
            }
            else
            {
                if (ImGui::Button("Stop Simulate", {(w - p), 0}))
                {
                    viewer->core().is_animating = false;
                }
            }
        }

        { // Step and Reset
            if (ImGui::Button("Step", {(w - p) * 0.5f, 0}))
            {
                if (!viewer->core().is_animating)
                {
                    m_isSingleStep = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset", {(w - p) * 0.5f, 0}))
            {
                Reset();
            }
        }

        { // Export current frame
            if (ImGui::Button("ExportPNG", {(w - p) * 0.5f, 0}))
            {
                ExportPNG();
            }
        }

        ImGui::End();
    }

    // https://github.com/Dreamtowards/Ethertia/blob/main/src/ethertia/imgui/Imgui_intl_draw.cpp#L1454
    static float RenderProfilerSection(const Util::Profiler::Section&                              sec,
                                       float                                                       x,
                                       float                                                       y,
                                       float                                                       full_width,
                                       float                                                       full_width_time,
                                       const std::function<float(const Util::Profiler::Section&)>& timefunc)
    {
        const float line_height = 16;

        double s_time  = timefunc(sec);
        double s_width = (s_time / full_width_time) * full_width;
        auto   rgba    = std::hash<std::string>()(sec.name) * 256;
        ImU32  col     = ImGui::GetColorU32(
            {((rgba >> 24) & 0xFF) / 255.0f, ((rgba >> 16) & 0xFF) / 255.0f, ((rgba >> 8) & 0xFF) / 255.0f, 1.0f});

        ImVec2 min = {x, y};
        ImVec2 max = {static_cast<float>(min.x + s_width), min.y + line_height};
        ImGui::RenderFrame(min, max, col);
        ImGui::RenderText(min, sec.name.c_str());

        if (s_width > 180)
        {
            std::string str       = std::to_string(s_time * 1000.0) + std::string("ms@") + std::to_string(sec.num_exec);
            ImVec2      text_size = ImGui::CalcTextSize(str.c_str());
            ImGui::RenderText({max.x - text_size.x, min.y}, str.c_str());
        }

        float dx = 0;
        for (const Util::Profiler::Section& sub_sec : sec.sections)
        {
            dx += RenderProfilerSection(sub_sec, x + dx, y + line_height, s_width, s_time, timefunc);
        }

        if (ImGui::IsWindowFocused() && ImGui::IsMouseHoveringRect(min, max))
        {
            ImGui::SetTooltip(
                "%s\n"
                "\n"
                "avg %fms\n"
                "las %fms\n"
                "sum %fms\n"
                "exc %u\n"
                "%%p  %f",
                sec.name.c_str(),
                sec.avg_time * 1000.0f,
                sec.last_time * 1000.0f,
                sec.sum_time * 1000.0f,
                (uint32_t)sec.num_exec,
                (float)(sec.parent ? sec.sum_time / sec.parent->sum_time : std::numeric_limits<float>::quiet_NaN()));
        }

        return s_width;
    }
    // https://github.com/Dreamtowards/Ethertia/blob/main/src/ethertia/imgui/Imgui_intl_draw.cpp#L1500
    void FSViewer::ProfilerWindow()
    {
        ImGui::Begin("Profiler");
        static int                                     s_selected_profiler_idx = 0;
        static std::pair<const char*, Util::Profiler*> profilers[]             = {
            {"Frame", &g_FrameProfiler},
            {"Simulator PreCompute", &g_PreComputeProfiler},
            {"Simulator Step", &g_StepProfiler},
        };
        Util::Profiler& prof = *profilers[s_selected_profiler_idx].second;

        static int                                                                              s_selected_time_func = 0;
        static std::pair<const char*, std::function<float(const Util::Profiler::Section& sec)>> s_time_funcs[]       = {
            {"SumTime", [](const Util::Profiler::Section& sec) { return sec.sum_time; }},
            {"LastTime", [](const Util::Profiler::Section& sec) { return sec.last_time; }},
            {"AvgTime", [](const Util::Profiler::Section& sec) { return sec.avg_time; }},
        };

        ImGui::SetNextItemWidth(200);
        if (ImGui::BeginCombo("###Profiler", profilers[s_selected_profiler_idx].first))
        {
            for (int i = 0; i < std::size(profilers); ++i)
            {
                if (ImGui::Selectable(profilers[i].first, s_selected_profiler_idx == i))
                {
                    s_selected_profiler_idx = i;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        if (ImGui::BeginCombo("###ProfilerTimeFunc", s_time_funcs[s_selected_time_func].first))
        {
            for (int i = 0; i < std::size(s_time_funcs); ++i)
            {
                if (ImGui::Selectable(s_time_funcs[i].first, s_selected_time_func == i))
                {
                    s_selected_time_func = i;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();

        if (ImGui::Button("Reset"))
        {
            prof.laterClearRootSection();
        }

        { // Profiler Section
            const Util::Profiler::Section& sec      = prof.getRootSection();
            auto&                          timefunc = s_time_funcs[s_selected_time_func].second;
            ImVec2                         begin    = {ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x,
                                                       ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y + 20};
            float                          width    = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
            RenderProfilerSection(sec, begin.x, begin.y, width, timefunc(sec), timefunc);
        }
        ImGui::End();

        // reset step profiler each frame
        if (viewer->core().is_animating)
        {
            g_StepProfiler.laterClearRootSection();
        }
    }

} // namespace FS