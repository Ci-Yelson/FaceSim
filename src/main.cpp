#include <iostream>

#include "Util/Profiler.hpp"
#include "Core/FSViewer.hpp"
#include "igl/opengl/ViewerData.h"
#include "igl/opengl/glfw/Viewer.h"

#include "igl/read_triangle_mesh.h"
#include <igl/remove_duplicate_vertices.h>

#include <memory>
#include <spdlog/spdlog.h>

Util::Profiler            g_FrameProfiler;
Util::Profiler            g_StepProfiler;
Util::Profiler            g_PreComputeProfiler;
igl::opengl::glfw::Viewer g_Viewer;

void LoadMesh(Eigen::MatrixXd& V, Eigen::MatrixXi& F, std::string meshURL)
{
    spdlog::info("Loading mesh from {}", meshURL);
    std::string file_suffix = meshURL.substr(meshURL.find_last_of('.') + 1);
    if (file_suffix == "stl")
    {
        std::ifstream input(meshURL, std::ios::in | std::ios::binary);
        if (!input)
        {
            spdlog::error("Failed to open {}", meshURL);
            exit(1);
        }
        Eigen::MatrixXd v, n;
        Eigen::MatrixXi f;
        bool            success = igl::readSTL(input, v, f, n);
        input.close();
        if (!success)
        {
            spdlog::error("Failed to read {}", meshURL);
            exit(1);
        }
        // remove duplicate vertices
        Eigen::Matrix<double, -1, 1> svi, svj;
        Eigen::MatrixXd s_vd;
        igl::remove_duplicate_vertices(v, 0, s_vd, svi, svj);
        std::for_each(f.data(), f.data() + f.size(), [&svj](int& f) { f = svj(f); });
        V = s_vd.cast<double>();
        F = f.cast<int>();
        // info
        std::cout << "Original vertices: " << V.rows() << std::endl;
        std::cout << "Duplicate-free vertices: " << F.rows() << std::endl;
    }
    else if (file_suffix == "obj")
    {
        Eigen::MatrixXd v;
        Eigen::MatrixXi f;
        if (!igl::readOBJ(meshURL, v, f))
        {
            spdlog::error("Failed to read {}", meshURL);
            exit(1);
        }
        V = v.cast<double>();
        F = f.cast<int>();
    }
    else if (file_suffix == "off")
    {
        igl::read_triangle_mesh(meshURL, V, F);
    }

    spdlog::info("Mesh loaded: {} vertices, {} faces", V.rows(), F.rows());
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "USAGE: [.EXE] [MESHURL]" << std::endl;
        return -1;
    }
    std::string mesh_url = argv[1];

    std::shared_ptr<FS::FSViewer> viewer_plugin = std::make_shared<FS::FSViewer>();
    LoadMesh(viewer_plugin->m_V, viewer_plugin->m_F, mesh_url);
    g_Viewer.data().set_mesh(viewer_plugin->m_V, viewer_plugin->m_F);
    g_Viewer.plugins.push_back(viewer_plugin.get());

    g_Viewer.core().is_animating      = false;
    g_Viewer.core().animation_max_fps = 30;
    g_Viewer.core().background_color << 100. / 255, 100. / 255, 100. / 255, 1.0;
    g_Viewer.launch_init(false, "FS Viewer");

    viewer_plugin->Setup();

    g_Viewer.launch_rendering();
    g_Viewer.launch_shut();

    return 0;
}