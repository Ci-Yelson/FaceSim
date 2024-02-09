cmake_minimum_required(VERSION 3.22.1)
project(FaceSim LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)
# For clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Use ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    set(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}")
endif()

##################################################################
##################################################################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
add_compile_definitions(EIGEN_DONT_PARALLELIZE)

add_executable(${PROJECT_NAME})
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_sources(${PROJECT_NAME} PUBLIC ${SRC_FILES})

# Add cmake/
list(PREPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
# libigl
find_package(libigl REQUIRED)
igl_include(glfw)
igl_include(copyleft tetgen)
igl_include(stb)
igl_include(xml)
target_link_libraries(${PROJECT_NAME} PUBLIC igl::glfw igl_copyleft::tetgen igl::stb igl::xml)
target_compile_definitions(${PROJECT_NAME} PUBLIC IGL_VIEWER_VIEWER_QUIET)
# Add libs/ : imgui_docking, imguizmo, stb
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/libs)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/libs/imgui)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/libs/imguizmo)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/libs/stb)
target_sources(${PROJECT_NAME} PUBLIC 
    ${PROJECT_SOURCE_DIR}/libs/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_tables.cpp
    ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/libs/imgui/backends/imgui_impl_glfw.cpp
    ${PROJECT_SOURCE_DIR}/libs/imgui/backends/imgui_impl_opengl3.cpp

    ${PROJECT_SOURCE_DIR}/libs/imguizmo/ImGuizmo.cpp
)
# OpenMP
find_package(OpenMP REQUIRED)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp:llvm")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
# spdlog
find_package(spdlog REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog_header_only)

# Test
set(TestName "WindingNumber")
add_executable(${TestName})
target_sources(${TestName} PUBLIC ${PROJECT_SOURCE_DIR}/test/702_WindingNumber.cpp)
target_include_directories(${TestName} PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(${TestName} PUBLIC igl::glfw)
