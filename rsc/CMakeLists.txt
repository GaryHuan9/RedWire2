#Setup
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/null.cpp" "")
target_sources(RedWire2.Resources PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/null.cpp")

#Assets
file(GLOB_RECURSE REDWIRE2_ASSETS Assets/**/*)

add_custom_command(OUTPUT REDWIRE2_ASSETS POST_BUILD COMMAND
        ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_SOURCE_DIR}/rsc/Assets
        ${CMAKE_BINARY_DIR}/Assets
)

add_custom_target(RedWire2.Assets DEPENDS REDWIRE2_ASSETS)
add_dependencies(RedWire2.Resources RedWire2.Assets)

#Shaders
set(REDWIRE2_VERT
        Shaders/Default.vert
        Shaders/ImGui.vert
)

set(REDWIRE2_FRAG
        Shaders/Default.frag
        Shaders/ImGui.frag
)

set(REDWIRE2_SHADERS "${CMAKE_BINARY_DIR}/Shaders")

bgfx_compile_shaders(
        TYPE VERTEX
        SHADERS ${REDWIRE2_VERT}
        VARYING_DEF ${CMAKE_SOURCE_DIR}/rsc/Shaders/varying.def.sc
        OUTPUT_DIR "${REDWIRE2_SHADERS}"
        INCLUDE_DIRS ${bgfx_SOURCE_DIR}/bgfx/src
)

bgfx_compile_shaders(
        TYPE FRAGMENT
        SHADERS ${REDWIRE2_FRAG}
        VARYING_DEF ${CMAKE_SOURCE_DIR}/rsc/Shaders/varying.def.sc
        OUTPUT_DIR "${REDWIRE2_SHADERS}"
        INCLUDE_DIRS ${bgfx_SOURCE_DIR}/bgfx/src
)

add_library(RedWire2.Shaders ${REDWIRE2_VERT} ${REDWIRE2_FRAG})
set_target_properties(RedWire2.Shaders PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(RedWire2.Shaders PUBLIC "${REDWIRE2_SHADERS}")
add_dependencies(RedWire2.Resources RedWire2.Shaders)
