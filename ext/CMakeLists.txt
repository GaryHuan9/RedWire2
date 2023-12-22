set(SFML_BUILD_AUDIO FALSE)
set(SFML_BUILD_NETWORK FALSE)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

add_subdirectory(SFML)

target_link_libraries(RedWire2.Core
        sfml-system
        sfml-window
        sfml-graphics
)

set(IMGUI_DIR ../imgui)
set(IMGUI_SFML_FIND_SFML OFF)
set(IMGUI_SFML_IMGUI_DEMO ON)
add_subdirectory(imgui-sfml)

target_link_libraries(RedWire2.Core ImGui-SFML::ImGui-SFML)
target_include_directories(RedWire2.Core PRIVATE include)

add_subdirectory(glew)
target_link_libraries(RedWire2.Core libglew_shared)
