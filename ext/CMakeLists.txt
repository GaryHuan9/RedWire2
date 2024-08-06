set(ONLY_LIBS ON)
set(BUILD_SHARED_LIBS ON)
set(glew-cmake_BUILD_STATIC OFF)
add_subdirectory(glew)
target_link_libraries(RedWire2.Core PUBLIC libglew_shared)

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_DOCUMENT_INTERNALS OFF)
set(GLFW_INSTALL OFF)

add_subdirectory(glfw)
add_subdirectory(imgui)

target_link_libraries(RedWire2.Core PUBLIC glfw)
target_link_libraries(RedWire2.Core PUBLIC imgui imgui-freetype imgui-stdlib imgui-glfw imgui-opengl3)
