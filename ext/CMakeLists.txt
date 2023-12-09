set(SFML_BUILD_AUDIO FALSE)
set(SFML_BUILD_NETWORK FALSE)

add_subdirectory(SFML)

target_link_libraries(RedWire2
        sfml-system
        sfml-window
        sfml-graphics
)

set(IMGUI_DIR ../imgui)
set(IMGUI_SFML_FIND_SFML OFF)
set(IMGUI_SFML_IMGUI_DEMO ON)
add_subdirectory(imgui-sfml)

target_link_libraries(RedWire2 ImGui-SFML::ImGui-SFML)
target_include_directories(RedWire2 PRIVATE include)
