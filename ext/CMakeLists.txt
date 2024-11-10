include(FetchContent)
set(ONLY_LIBS ON)
set(BUILD_SHARED_LIBS ON)

#glfw
fetchcontent_declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        FIND_PACKAGE_ARGS NAMES glfw3
)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_DOCUMENT_INTERNALS OFF)
set(GLFW_INSTALL OFF)

fetchcontent_makeavailable(glfw)
target_link_libraries(RedWire2.Core PRIVATE glfw)

#Dear ImGui
fetchcontent_declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.3-docking
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

fetchcontent_makeavailable(imgui)
file(GLOB IMGUI_SOURCES CONFIGURE_DEPENDS ${imgui_SOURCE_DIR}/*.cpp)
add_library(imgui_build ${IMGUI_SOURCES})
target_include_directories(imgui_build INTERFACE ${imgui_SOURCE_DIR})

target_link_libraries(RedWire2.Core PRIVATE imgui_build)
target_sources(RedWire2.Core PRIVATE
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

#bgfx
fetchcontent_declare(
        bgfx
        GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
        GIT_TAG v1.128.8832-486
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

fetchcontent_makeavailable(bgfx)
target_link_libraries(RedWire2.Core PRIVATE bgfx)
